#include "XeroPathGen.h"
#include "GameFieldManager.h"
#include "build.h"
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtWidgets/QApplication>
#include <QtWidgets/QSplashScreen>
#include <QtWidgets/QMessageBox>
#include <fstream>
#include <sstream>
#include <iostream>
#include <thread>


std::stringstream log2stream;

void myMessageOutput(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
	QByteArray localMsg = msg.toLocal8Bit();
	const char* file = context.file ? context.file : "";
	const char* function = context.function ? context.function : "";
	switch (type) {
	case QtDebugMsg:
		log2stream << "Debug: ";
		break;
	case QtInfoMsg:
		log2stream << "Info: ";
		break;
	case QtWarningMsg:
		log2stream << "Warning: ";
		break;
	case QtCriticalMsg:
		log2stream << "Critical: ";
		break;
	case QtFatalMsg:
		log2stream << "Error: ";
		break;
	}
	log2stream << localMsg.constData() << std::endl;
}

static void pruneLogFiles(QDir& logdir)
{
	int count = 0;
	QStringList paths;
	QStringList list;
	list << "*.log";
	QFileInfoList files = logdir.entryInfoList(list, QDir::Filter::NoFilter, QDir::SortFlag::Time);
	while (files.size() > 20)
	{
		QFile file(files.back().absoluteFilePath());
		file.remove();
		files.pop_back();
		count++;
	}

	qDebug() << "Pruning old log files, " << count << "deleted";
}

int main(int argc, char *argv[])
{
	qInstallMessageHandler(myMessageOutput);
	qDebug() << "starting XeroPathGen";

	QCoreApplication::setOrganizationName("ErrorCodeXero");
	QCoreApplication::setOrganizationDomain("www.wilsonvillerobotics.com");
	QCoreApplication::setApplicationName("XeroPathGenerator");
	QCoreApplication::setApplicationVersion("1.0.0");

	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QApplication a(argc, argv);

	QString exedir = QCoreApplication::applicationDirPath();
	QString imagepath = exedir + "/images/Splash.png";
	QPixmap image(imagepath);

	std::string error;

	QSplashScreen splash(image);
	QFont font = splash.font();
	font.setPointSizeF(16.0);
	splash.setFont(font);
	splash.show();
	splash.showMessage("Initializing ...");
	a.processEvents();

	QStringList dirs = QStandardPaths::standardLocations(QStandardPaths::AppLocalDataLocation);
	QString appdir = dirs.front();

	GameFieldManager fields;
	RobotManager robots;

	argc--;
	argv++;

	while (argc-- > 0)
	{
		std::string arg = *argv++;
		if (arg == "--fields")
		{
			if (argc == 0)
			{
				QMessageBox box(QMessageBox::Icon::Critical,
					"Error", "--fields flag requires path argument", QMessageBox::StandardButton::Ok);
				box.exec();
				return -1;
			}

			fields.setDirectory(*argv++);
			argc--;
		}
		else if (arg == "--robots")
		{
			if (argc == 0)
			{
				QMessageBox box(QMessageBox::Icon::Critical,
					"Error", "--robots flag requires path argument", QMessageBox::StandardButton::Ok);
				box.exec();
				return -1;
			}

			robots.setDirectory(*argv++);
			argc--;
		}
	}

	fields.addDefaultDirectory((appdir + "/fields"));
	robots.addDefaultDirectory((appdir + "/robots"));
	std::chrono::milliseconds delay(250);
	fields.copyDefaults("fields");

	splash.showMessage("Initializing fields ...");
	a.processEvents();
	if (!fields.initialize())
	{
		QMessageBox box(QMessageBox::Icon::Critical, "Error",
			"Could not initialize the game field manager", QMessageBox::StandardButton::Ok);
		box.exec();
		return -1;
	}
	std::this_thread::sleep_for(delay);

	splash.showMessage("Initializing robots ...");
	a.processEvents();
	if (!robots.initialize())
	{
		QMessageBox box(QMessageBox::Icon::Critical, "Error",
			"Could not initialize the robots manager", QMessageBox::StandardButton::Ok);
		box.exec();
		return -1;
	}
	std::this_thread::sleep_for(delay);

	fields.dumpSearchPath("Fields");
	robots.dumpSearchPath("Robots");

	try {
		XeroPathGen w(robots, fields, log2stream);
		w.show();
		splash.finish(&w);
		return a.exec();
	}
	catch (const std::exception& ex)
	{
		qDebug() << "Exception Caught: " << ex.what();
		return -1;
	}
}
