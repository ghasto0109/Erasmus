#ifndef CONFIGURATION_FILE_H_
#define CONFIGURATION_FILE_H_

#include <QtCore>

class ConfigurationFile
{
public:
	ConfigurationFile();

	bool loadFromIniFile(const QString& filePath_ini);
	bool saveToIniFile(const QString& filePath_ini);
	bool saveToDefaultIniFile(const QString& filePath_ini);

	const std::string& errorMessage() const { return _errorMessage; }

	// property: nAlines, nScans
	int nAlines() const { return _nAlines; }
	int nScans() const { return _nScans; }
	int sampleCountInFrame() const { return 2 * nAlines() * nScans(); } // x 2 : two channel
	int byteSizeInFrame() const { return sizeof(uint16_t) * sampleCountInFrame(); }

	const QString processTypeString() const { return _processTypeString; }

	const QString& iniFilePath() const { return filePath_ini; }
	const QString& dataFilePath() const { return filePath_data; }
	const QString& calibrationFilePath() const { return filePath_calibration; }
	const QString& backgroundFilePath() const { return filePath_background; }

private:
	int _nScans, _nAlines;
	QString _processTypeString;
	QString filePath_ini, filePath_data, filePath_calibration, filePath_background;

	std::string _errorMessage;
};

#endif // CONFIGURATION_FILE_H_