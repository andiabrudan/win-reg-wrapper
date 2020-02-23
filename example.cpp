#include <Windows.h>
#include "./registry.h"

class voidbuff : public std::streambuf {};
voidbuff nostreambuff;
std::ostream devnull(&nostreambuff);
const int LOG_LEVEL = 0;
inline std::ostream& log(int level)
{
	if (level >= LOG_LEVEL)
		return std::cout;
	else
		return devnull;
}

void ChangeRegirtyNumber(HKEY machine, std::string_view key, std::string_view value, DWORD data)
{
	try
	{
		log(1) << "Attempting to change: " << reg::except::toString(machine, key, value) << "; desired data: " << data << "\n";
		if (reg::key_exists(machine, key))
		{
			log(2) << "Key \"" << key << "\" already exists\n";
			if (reg::value_exists(machine, key, value))
			{
				DWORD old_data = reg::query::number(machine, key, value);
				log(2) << "Value \"" << value << "\" already exists; current data: " << old_data << "\n";
				if (old_data != data)
				{
					reg::update::number(machine, key, value, data);
					log(4) << "Successfully updated " << value << " (was " << old_data << ")\n";
				}
				else
				{
					log(3) << "No change needed for \"" << value << "\"\n";
				}
			}
			else
			{
				log(2) << "Value \"" << value << "\" does not exist\n";
				reg::create::number(machine, key, value, data);
				log(4) << "Successfully created new value \"" << value << "\" and assigned data: " << data << "\n";
			}
		}
		else
		{
			log(2) << "Key \"" << key << "\" does not exist\n";
			reg::create::key(machine, key);
			log(2) << "Creating value \"" << value << "\"\n";
			reg::create::number(machine, key, value, data);
			log(4) << "Successfully created new value \"" << value << "\" and assigned data: " << data << "\n";
		}
	}
	catch (const std::exception & e)
	{
		log(2) << "An exception occurred while trying to edit \"" << reg::except::toString(machine, key, value) << "\"\n";
		log(4) << e.what() << " --- while trying to edit value \"" << value << "\"\n";
	}
	log(4) << "\n";
}

void ChangeRegirtyString(HKEY machine, std::string_view key, std::string_view value, std::string_view data)
{
	try
	{
		log(1) << "Attempting to change: " << reg::except::toString(machine, key, value) << "; desired data: \"" << data << "\"\n";
		if (reg::key_exists(machine, key))
		{
			log(2) << "Key \"" << key << "\" already exists\n";
			if (reg::value_exists(machine, key, value))
			{
				std::string old_data = reg::query::string(machine, key, value);
				log(2) << "Value \"" << value << "\" already exists; current data: \"" << old_data << "\"\n";
				if (old_data != data)
				{
					reg::update::string(machine, key, value, data);
					log(4) << "Successfully updated " << value << " (was \"" << old_data << "\")\n";
				}
				else
				{
					log(3) << "No change needed for \"" << value << "\"\n";
				}
			}
			else
			{
				log(2) << "Value \"" << value << "\" does not exist\n";
				reg::create::string(machine, key, value, data);
				log(4) << "Successfully created new value \"" << value << "\" and assigned data: \"" << data << "\"\n";
			}
		}
		else
		{
			log(2) << "Key \"" << key << "\" does not exist\n";
			reg::create::key(machine, key);
			log(2) << "Creating value \"" << value << "\"\n";
			reg::create::string(machine, key, value, data);
			log(4) << "Successfully created new value \"" << value << "\" and assigned data: \"" << data << "\"\n";
		}
	}
	catch (const std::exception & e)
	{
		log(2) << "An exception occurred while trying to edit \"" << reg::except::toString(machine, key, value) << "\"\n";
		log(4) << e.what() << " --- while trying to edit value \"" << value << "\"\n";
	}
	log(4) << "\n";
}

#define NOTEPAD_EXEC	"notepad.exe %1\0"
#define NOTEPAD_ADDON "*\\shell\\Open with Notepad\\command"
#define EXPLORER_REGISTER_PATH	"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"
#define EXPLORER_THISPC	"LaunchTo"
int main()
{
    // Example adds "Open with notepad" to right-click context menu
    ChangeRegirtyString(HKEY_CLASSES_ROOT, NOTEPAD_ADDON, "", NOTEPAD_EXEC);

    // Example sets Windows Explorer to open by default to "This PC" rather than "Quick Explorer"
    ChangeRegirtyNumber(HKEY_CURRENT_USER, EXPLORER_REGISTER_PATH, EXPLORER_THISPC, 1);
    
}