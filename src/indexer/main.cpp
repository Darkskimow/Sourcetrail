
#include "language_packages.h"
#include "AppPath.h"
#include "ApplicationSettings.h"
#include "FileLogger.h"
#include "InterprocessIndexer.h"
#include "LanguagePackageManager.h"
#include "LogManager.h"
#include "UserPaths.h"
#include "logging.h"
#include "setupApp.h"

#if BUILD_CXX_LANGUAGE_PACKAGE
#	include "LanguagePackageCxx.h"
#endif	  // BUILD_CXX_LANGUAGE_PACKAGE

#if BUILD_JAVA_LANGUAGE_PACKAGE
#	include "LanguagePackageJava.h"
#endif	  // BUILD_JAVA_LANGUAGE_PACKAGE

#if BOOST_OS_WINDOWS
	#include <Windows.h>
#endif

//--------------------------------------------------------------------------------------------------
void setupLogging(const FilePath& logFilePath)
{
	LogManager* logManager = LogManager::getInstance().get();

	// std::shared_ptr<ConsoleLogger> consoleLogger = std::make_shared<ConsoleLogger>();
	// // consoleLogger->setLogLevel(Logger::LOG_WARNINGS | Logger::LOG_ERRORS);
	// consoleLogger->setLogLevel(Logger::LOG_ALL);
	// logManager->addLogger(consoleLogger);

	// L'indexeur secondaire journalise uniquement dans le fichier partage par le processus parent.
	std::shared_ptr<FileLogger> fileLogger = std::make_shared<FileLogger>();
	fileLogger->setLogFilePath(logFilePath);
	fileLogger->setLogLevel(Logger::LOG_ALL);
	logManager->addLogger(fileLogger);
}

//--------------------------------------------------------------------------------------------------
void suppressCrashMessage()
{
#if BOOST_OS_WINDOWS
	// Evite l'affichage des boites de dialogue systeme dans le sous-processus d'indexation.
	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
#endif
}

//--------------------------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
	setupDefaultLocale();
	
	ProcessId processId = ProcessId::INVALID;
	std::string instanceUuid;
	std::string appPath;
	std::string userDataPath;
	std::string logFilePath;

	// Le processus principal transmet les arguments dans un ordre fixe.
	if (argc >= 2)
	{
		processId = ProcessId(std::stoi(argv[1]));
	}

	if (argc >= 3)
	{
		instanceUuid = argv[2];
	}

	if (argc >= 4)
	{
		appPath = argv[3];
	}

	if (argc >= 5)
	{
		userDataPath = argv[4];
	}

	if (argc >= 6)
	{
		logFilePath = argv[5];
	}

	AppPath::setSharedDataDirectoryPath(FilePath(appPath));
	UserPaths::setUserDataDirectoryPath(FilePath(userDataPath));

	// Le chemin de log est optionnel pour permettre des executions minimales si besoin.
	if (!logFilePath.empty())
	{
		setupLogging(FilePath(logFilePath));
	}

	suppressCrashMessage();

	ApplicationSettings* appSettings = ApplicationSettings::getInstance().get();
	appSettings->load(UserPaths::getAppSettingsFilePath());
	LogManager::getInstance()->setLoggingEnabled(appSettings->getLoggingEnabled());

	LOG_INFO("sharedDataPath: " + AppPath::getSharedDataDirectoryPath().str());
	LOG_INFO("userDataPath: " + UserPaths::getUserDataDirectoryPath().str());

	// L'indexeur ne charge que les paquets de langages necessaires a l'analyse.
#if BUILD_CXX_LANGUAGE_PACKAGE
	LanguagePackageManager::getInstance()->addPackage(std::make_shared<LanguagePackageCxx>());
#endif	  // BUILD_CXX_LANGUAGE_PACKAGE

#if BUILD_JAVA_LANGUAGE_PACKAGE
	LanguagePackageManager::getInstance()->addPackage(std::make_shared<LanguagePackageJava>());
#endif	  // BUILD_JAVA_LANGUAGE_PACKAGE

	InterprocessIndexer indexer(instanceUuid, processId);
	indexer.work();

	return 0;
}
