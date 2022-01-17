#include "stdafx.h"
#include "ConfigurationFile.h"

ConfigurationFile::ConfigurationFile() :
	_nScans(1190), 
	_nAlines(1024), 
	_processTypeString("Intensity"), 
	filePath_data(""), 
	filePath_calibration("calibration.dat"), 
	filePath_background("bg.bin")
{
}

bool ConfigurationFile::loadFromIniFile(const QString& filePath_ini)
{
	this->filePath_ini = filePath_ini;

	QFileInfo fileInfo_ini(filePath_ini);
	if (false == fileInfo_ini.exists())
	{
		_errorMessage = "INI file does not exist: %s" + fileInfo_ini.filePath().toStdString();
		return false;
	}

	// Parse INI file
	QSettings settings(fileInfo_ini.filePath(), QSettings::IniFormat);

	// Process type string
	_processTypeString = settings.value("configuration/processType").toString().toLower();

	// nAlines, nScans
	_nAlines = settings.value("configuration/nAlines", -1).toInt();
	_nScans = settings.value("configuration/nScans", -1).toInt();

	if (_nAlines == -1)
	{
		_errorMessage = "configuration/nAlines is not specificated.";
		return false;
	}

	if (_nScans == -1)
	{
		_errorMessage = "configuration/nScans is not specificated.";
		return false;
	}

	// Data files
	QString path_data = settings.value("file/dataFile").toString();
	QString path_calibration = settings.value("file/calibrationFile").toString();
	QString path_background = settings.value("file/backgroundFile").toString();

	// Set file paths to be relative to ini file path
	QDir dir_ini = fileInfo_ini.dir();

	filePath_data = QFileInfo(dir_ini, path_data).filePath();
	filePath_calibration = QFileInfo(dir_ini, path_calibration).filePath();
	filePath_background = QFileInfo(dir_ini, path_background).filePath();

	return true;
}

bool ConfigurationFile::saveToIniFile(const QString& filePath_ini)
{
	QSettings settings(filePath_ini, QSettings::IniFormat);

	settings.setValue("configuration/nAlines", nAlines());
	settings.setValue("configuration/nScans", nScans());
	settings.setValue("configuration/processType", processTypeString());

	settings.setValue("file/dataFile", QFileInfo(filePath_ini).baseName() + ".data");
	settings.setValue("file/calibrationFile", QFileInfo(filePath_ini).baseName() + ".calibration");
	settings.setValue("file/backgroundFile", QFileInfo(filePath_ini).baseName() + ".background");

	settings.sync();
	return true;
}

bool ConfigurationFile::saveToDefaultIniFile(const QString& filePath_ini)
{
	QSettings settings(filePath_ini, QSettings::IniFormat);

	settings.setValue("configuration/nAlines", nAlines());
	settings.setValue("configuration/nScans", nScans());
	settings.setValue("configuration/processType", processTypeString());

	// TODO: You'd better change default calibration & background file name. 
	settings.setValue("file/calibrationFile", QFileInfo(filePath_calibration).fileName());
	settings.setValue("file/backgroundFile", QFileInfo(filePath_background).fileName());

	settings.sync();
	return true;
}
