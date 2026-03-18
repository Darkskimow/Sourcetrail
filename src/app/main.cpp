#include "setupApp.h"

#include "Application.h"
#include "ApplicationSettings.h"
#include "ApplicationSettingsPrefiller.h"
#include "CommandLineParser.h"
#include "ConsoleLogger.h"
#include "FileLogger.h"
#include "LanguagePackageManager.h"
#include "LogManager.h"
#include "MessageIndexingInterrupted.h"
#include "MessageLoadProject.h"
#include "MessageStatus.h"
#include "Platform.h"
#include "QtApplication.h"
#include "QtCoreApplication.h"
#include "QtNetworkFactory.h"
#include "QtViewFactory.h"
#include "ResourcePaths.h"
#include "ScopedFunctor.h"
#include "SourceGroupFactory.h"
#include "SourceGroupFactoryModuleCustom.h"
#include "language_packages.h"
#include "utilityQt.h"

#if BUILD_CXX_LANGUAGE_PACKAGE
	#include "LanguagePackageCxx.h"
	#include "SourceGroupFactoryModuleCxx.h"
#endif	  // BUILD_CXX_LANGUAGE_PACKAGE

#if BUILD_JAVA_LANGUAGE_PACKAGE
	#include "LanguagePackageJava.h"
	#include "SourceGroupFactoryModuleJava.h"
#endif	  // BUILD_JAVA_LANGUAGE_PACKAGE

#if BOOST_OS_WINDOWS
	#include <windows.h>
#endif

#include <QByteArray>
#include <QtEnvironmentVariables>

#include <csignal>
#include <iostream>

using namespace std;
using namespace boost::filesystem;

//--------------------------------------------------------------------------------------------------
void closeConsoleWindow()
{
#if BOOST_OS_WINDOWS
	// Masque la console creee par Windows lorsque Sourcetrail n'a pas ete lance depuis un terminal.
	if (HWND consoleWnd = GetConsoleWindow(); consoleWnd != nullptr) {
		DWORD consoleOwnerProcessId;
		if (GetWindowThreadProcessId(consoleWnd, &consoleOwnerProcessId) != 0) {
			if (consoleOwnerProcessId == GetCurrentProcessId()) {
				// Le masquage ne fonctionne pas si le terminal par defaut n'est pas le
				// "Windows console host", comme c'est le cas sur certaines configurations
				// de Windows 11. Voir https://github.com/petermost/Sourcetrail/issues/19.

				ShowWindow(consoleWnd, SW_HIDE);
			}
		}
	}
#endif
}

//--------------------------------------------------------------------------------------------------
void signalHandler(int  /*signum*/)
{
	std::cout << "interrupt indexing" << std::endl;
	MessageIndexingInterrupted().dispatch();
}

//--------------------------------------------------------------------------------------------------
void setupLogging()
{
	LogManager* logManager = LogManager::getInstance().get();

	// Active la sortie console pour attraper les messages emis tres tot au demarrage.
	std::shared_ptr<ConsoleLogger> consoleLogger = std::make_shared<ConsoleLogger>();
	consoleLogger->setLogLevel(Logger::LOG_ALL);
	logManager->addLogger(consoleLogger);

	// Conserve aussi une trace persistante sur disque pour les diagnostics post-mortem.
	std::shared_ptr<FileLogger> fileLogger = std::make_shared<FileLogger>();
	fileLogger->setLogLevel(Logger::LOG_ALL);
	fileLogger->deleteLogFiles(FileLogger::generateDatedFileName("log", "", -30));
	logManager->addLogger(fileLogger);
}

//--------------------------------------------------------------------------------------------------
void addLanguagePackages()
{
	// Les modules de groupes sources doivent etre declares avant les paquets de langages
	// afin que l'application sache creer les bons types de projets.
	SourceGroupFactory::getInstance()->addModule(std::make_shared<SourceGroupFactoryModuleCustom>());

#if BUILD_CXX_LANGUAGE_PACKAGE
	SourceGroupFactory::getInstance()->addModule(std::make_shared<SourceGroupFactoryModuleCxx>());
#endif	  // BUILD_CXX_LANGUAGE_PACKAGE

#if BUILD_JAVA_LANGUAGE_PACKAGE
	SourceGroupFactory::getInstance()->addModule(std::make_shared<SourceGroupFactoryModuleJava>());
#endif	  // BUILD_JAVA_LANGUAGE_PACKAGE

#if BUILD_CXX_LANGUAGE_PACKAGE
	LanguagePackageManager::getInstance()->addPackage(std::make_shared<LanguagePackageCxx>());
#endif	  // BUILD_CXX_LANGUAGE_PACKAGE

#if BUILD_JAVA_LANGUAGE_PACKAGE
	LanguagePackageManager::getInstance()->addPackage(std::make_shared<LanguagePackageJava>());
#endif	  // BUILD_JAVA_LANGUAGE_PACKAGE
}

//--------------------------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
	setupDefaultLocale();

	// Recupere toujours le bon dossier executable pour les differentes formes de lancement :
	// Windows : "Sourcetrail" (n'existe pas encore, donc canonical() echouerait)
	// Windows : "Sourcetrail.exe"
	// Linux   : "./Sourcetrail"

	const path appDirectory = weakly_canonical(argv[0]).parent_path();
	Version version = setupAppDirectories(appDirectory.generic_string());

	if constexpr (utility::Platform::isLinux())
	{
		if (qgetenv("SOURCETRAIL_VIA_SCRIPT").isNull())
		{
			std::cout << "ERROR: Please run Sourcetrail via the Sourcetrail.sh script!" << std::endl;
		}
	}
	MessageStatus("Starting Sourcetrail version "s + version.toDisplayString()).dispatch();
	MessageStatus("Setting application directory: "s + appDirectory.generic_string()).dispatch();

	commandline::CommandLineParser commandLineParser(version.toDisplayString());
	commandLineParser.preparse(argc, argv);
	if (commandLineParser.exitApplication())
	{
		return 0;
	}

	setupAppEnvironment(argc, argv);

	if (commandLineParser.runWithoutGUI())
	{
		// Mode sans interface graphique utilise pour les executions en ligne de commande.
		[[maybe_unused]]
		QtCoreApplication qtApp(argc, argv);

		setupLogging();

		Application::createInstance(version, nullptr, nullptr);
		
		[[maybe_unused]]
		ScopedFunctor f([]()
		{
			// Garantit la liberation de l'instance globale meme en cas de retour anticipe.
			Application::destroyInstance();
		});

		ApplicationSettingsPrefiller::prefillPaths(ApplicationSettings::getInstance().get());
		addLanguagePackages();

		signal(SIGINT, signalHandler);
		signal(SIGTERM, signalHandler);
		signal(SIGABRT, signalHandler);

		commandLineParser.parse();

		if (commandLineParser.exitApplication())
		{
			return 0;
		}

		if (commandLineParser.hasError())
		{
			std::cout << commandLineParser.getError() << std::endl;
		}
		else
		{
			MessageLoadProject(commandLineParser.getProjectFilePath(), false, commandLineParser.getRefreshMode()).dispatch();
		}

		return QtCoreApplication::exec();
	}
	else
	{
		// Mode graphique complet, avec vues Qt et integration reseau.
		[[maybe_unused]]
		QtApplication qtApp(argc, argv);

		setupLogging();

		QtViewFactory viewFactory;
		QtNetworkFactory networkFactory;

		Application::createInstance(version, &viewFactory, &networkFactory);
		
		[[maybe_unused]]
		ScopedFunctor f([]()
		{
			// Symetrique a la creation : l'application est detruite automatiquement a la sortie.
			Application::destroyInstance();
		});

		auto applicationSettings = ApplicationSettings::getInstance();
		ApplicationSettingsPrefiller::prefillPaths(applicationSettings.get());
		if (!applicationSettings->getLoggingEnabled())
			closeConsoleWindow();

		addLanguagePackages();

		utility::loadFontsFromDirectory(ResourcePaths::getFontsDirectoryPath(), ".otf");
		utility::loadFontsFromDirectory(ResourcePaths::getFontsDirectoryPath(), ".ttf");

		if (commandLineParser.hasError())
		{
			Application::getInstance()->handleDialog(commandLineParser.getError());
		}
		else
		{
			MessageLoadProject(commandLineParser.getProjectFilePath(), false, RefreshMode::NONE).dispatch();
		}

		return QtApplication::exec();
	}
}
