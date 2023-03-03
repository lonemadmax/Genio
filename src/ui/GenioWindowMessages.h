/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef GenioWindowMessages_H
#define GenioWindowMessages_H


// Self enum names begin with MSG_ and values are all lowercase
// External enum names begin with MODULENAME_ and values are Capitalized
// 'NOTI' temporary
enum {
	// Project menu
	MSG_PROJECT_CLOSE			= 'prcl',
	MSG_PROJECT_NEW				= 'prne',
	MSG_PROJECT_OPEN			= 'prop',
	MSG_PROJECT_SET_ACTIVE		= 'psac',	// TODO
	MSG_PROJECT_SETTINGS		= 'prse',
	MSG_PROJECT_FOLDER_OPEN		= 'pfop',

	// File menu
	MSG_FILE_NEW				= 'fine',
	MSG_FILE_OPEN				= 'fiop',
	MSG_FILE_SAVE				= 'fisa',
	MSG_FILE_SAVE_AS			= 'fsas',
	MSG_FILE_SAVE_ALL			= 'fsal',
	MSG_FILE_CLOSE				= 'ficl',
	MSG_FILE_CLOSE_ALL			= 'fcal',
	MSG_FILE_FOLD_TOGGLE		= 'fifo',

	// Edit menu
	MSG_TEXT_DELETE				= 'tede',
	MSG_TEXT_OVERWRITE			= 'teov',
	MSG_WHITE_SPACES_TOGGLE		= 'whsp',
	MSG_LINE_ENDINGS_TOGGLE		= 'lien',
	MSG_EOL_CONVERT_TO_UNIX		= 'ectu',
	MSG_EOL_CONVERT_TO_DOS		= 'ectd',
	MSG_EOL_CONVERT_TO_MAC		= 'ectm',
	MSG_EOL_SET_TO_UNIX			= 'estu',
	MSG_EOL_SET_TO_DOS			= 'estd',
	MSG_EOL_SET_TO_MAC			= 'estm',
	
	MSG_AUTOCOMPLETION			= 'auto',	
	MSG_FORMAT					= 'form',
	MSG_GOTODEFINITION			= 'gode',
	MSG_GOTODECLARATION			= 'gocl',
	MSG_GOTOIMPLEMENTATION		= 'goim',
	MSG_SWITCHSOURCE			= 'swit',
	MSG_SIGNATUREHELP			= 'sihe',

	// view
	MSG_VIEW_ZOOMIN				= 'zoin',
	MSG_VIEW_ZOOMOUT			= 'zoou',
	MSG_VIEW_ZOOMRESET			= 'zore',

	// Search menu & group
	MSG_FIND_GROUP_SHOW			= 'figs',
	MSG_FIND_MENU_SELECTED		= 'fmse',
	MSG_FIND_PREVIOUS			= 'fipr',
	MSG_FIND_MARK_ALL			= 'fmal',
	MSG_FIND_NEXT				= 'fite',
	MSG_REPLACE_GROUP_SHOW		= 'regs',
	MSG_REPLACE_MENU_SELECTED 	= 'rmse',
	MSG_REPLACE_ONE				= 'reon',
	MSG_REPLACE_NEXT			= 'rene',
	MSG_REPLACE_PREVIOUS		= 'repr',
	MSG_REPLACE_ALL				= 'real',
	MSG_GOTO_LINE				= 'goli',
	MSG_BOOKMARK_CLEAR_ALL		= 'bcal',
	MSG_BOOKMARK_GOTO_NEXT		= 'bgne',
	MSG_BOOKMARK_GOTO_PREVIOUS	= 'bgpr',
	MSG_BOOKMARK_TOGGLE			= 'book',

	// Build menu
	MSG_BUILD_PROJECT			= 'bupr',
	MSG_BUILD_PROJECT_STOP		= 'bpst',
	MSG_CLEAN_PROJECT			= 'clpr',
	MSG_RUN_TARGET				= 'ruta',
	MSG_BUILD_MODE_RELEASE		= 'bmre',
	MSG_BUILD_MODE_DEBUG		= 'bmde',
	MSG_DEBUG_PROJECT			= 'depr',
	MSG_MAKE_CATKEYS			= 'maca',
	MSG_MAKE_BINDCATALOGS		= 'mabi',

	// Scm menu
	MSG_GIT_COMMAND				= 'gitc',
	MSG_HG_COMMAND				= 'hgco',

	// Window menu
	MSG_WINDOW_SETTINGS			= 'wise',
	MSG_TOGGLE_TOOLBAR			= 'toto',

	
	// Toolbar
	MSG_BUFFER_LOCK					= 'bulo',
	MSG_BUILD_MODE					= 'bumo',
	MSG_FILE_MENU_SHOW				= 'fmsh',
	MSG_FILE_NEXT_SELECTED			= 'fnse',
	MSG_FILE_PREVIOUS_SELECTED		= 'fpse',
	MSG_FIND_GROUP_TOGGLED			= 'figt',
	MSG_FIND_IN_FILES				= 'fifi',
	MSG_RUN_CONSOLE_PROGRAM_SHOW	= 'rcps',
	MSG_RUN_CONSOLE_PROGRAM			= 'rcpr',
	MSG_LINE_TO_GOTO				= 'ltgt',
	MSG_REPLACE_GROUP_TOGGLED		= 'regt',
	MSG_SHOW_HIDE_PROJECTS			= 'shpr',
	MSG_SHOW_HIDE_OUTPUT			= 'shou',

	MSG_SELECT_TAB					= 'seta'
};

#endif // GenioWindowMessages_H