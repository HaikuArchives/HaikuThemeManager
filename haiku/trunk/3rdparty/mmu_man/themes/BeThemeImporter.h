#ifndef _BE_THEME_IMPORTER_H
#define _BE_THEME_IMPORTER_H
/*
 * BeTheme Importer class header
 */

#include <Query.h>

#include "ThemeImporter.h"

namespace Z {
namespace ThemeManager {

class BeThemeImporter : public ThemeImporter {
public:
			BeThemeImporter();
virtual		~BeThemeImporter();
	
	/* presentation */
	
virtual const char	*Description(); /* tooltip... */
	
	/* Theme manipulation */
	
virtual status_t	FetchThemes();
virtual status_t	ImportNextTheme(BMessage **theme);
virtual status_t	EndImports();
	
	/*  */
	
private:

friend class Z::ThemeManager::ThemeManager;
	BQuery		fQuery;
};

} // ns ThemeManager
} // ns Z

using namespace Z::ThemeManager;

#endif /* _BE_THEME_IMPORTER_H */
