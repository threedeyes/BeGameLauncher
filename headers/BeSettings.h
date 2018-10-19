#ifndef BESETTINGS_H
#define BESETTINGS_H

#include <Message.h>
#include <String.h>

class BeSettings : public BMessage
{
	BString pathToSettingsFile;

public:
	BeSettings(const char *fileName);
	virtual ~BeSettings();

	bool DumpSettingsToFile(void);
	bool ReadSettingsFromFile(void);

	const char *GetString(const char *name) const;
	void SetString(const char *name, const char *string);
};

#endif // BESETTINGS_H
