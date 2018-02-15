/*------------------------------------------------------------------------------
 * CppToolbox: TextTemplate
 * https://github.com/pirminschmid/CppToolbox
 *
 * Copyright (c) 2017, Pirmin Schmid, MIT license
 *----------------------------------------------------------------------------*/

#ifndef TOOLBOX_TEXTTEMPLATE_H_
#define TOOLBOX_TEXTTEMPLATE_H_

#include <ostream>
#include <unordered_map>
#include <string>

#include "KeyValueStore.h"

namespace toolbox {

	/**
	 * simple text/string template class that implements 2 forms of template variables at the moment
	 * - double_brackets_style: variable var is encapsulated in {{var}}, no exceptions
	 *   this is the preferred / default style, since it is simpler and faster
	 *
	 * - dollar_sign_style: follows the rules as defined by string.Template() in Python (from Python documentation)
	 *   - $$ is an escape; it is replaced with a single $.
	 *   - $identifier names a substitution placeholder matching a mapping key of "identifier".
	 *   - ${identifier} is equivalent to $identifier. It is required when valid identifier characters
	 *     follow the placeholder but are not part of the placeholder, such as "${noun}ification".
	 *   note: there is also a slight difference compared to the Python library: In that library, the lookup
	 *   was driven by the identifiers found in the template, which leads to a stop of the program
	 *   if such an id is missing in the substitutions map. Here, the variable is just not substituted.
	 *
	 * all variable identifiers are case sensitive
	 *
	 * the map with substitutions[varname] = content comes without {{}} or $ for varname
	 * content should not have strings that look like identifiers. Due to unforeseeable order of
	 * replacement, the result is undefined.
	 *
	 * limitations: not all (or not many) bells and whistles are implemented yet, and not every potential corner
	 * case is tested at the moment (e.g. checks on unfortunate identifier names). No speed optimization.
	 * This simple version at the moment is just tested to be "good enough" for some simple templating
	 * needs at the moment. It does *not* intend to replace full fledged templating libraries.
	 *
	 * v1.1 2017-04-19 / 2017-05-03, Pirmin Schmid
	 */
	class TextTemplate {
	public:
		enum Style {
			kDoubleBracketsStyle,
			kDollarSignStyle
		};

		TextTemplate(const Style style = kDefaultStyle);

		bool readFile(const std::string &filename);
		bool writeFile(const std::string &filename) const;

		void setContent(const std::string &content);
		const std::string &getContent() const;

		void print(std::ostream *out) const;

		void substituteWith(const toolbox::KeyValueStore &substitutions);

		static constexpr Style kDefaultStyle = kDoubleBracketsStyle;

	private:
		Style style_;
		std::string content_;

		void substituteWith_DoubleBracketsStyle(const toolbox::KeyValueStore &substitutions);
		void substituteWith_PythonDollarStyle(const toolbox::KeyValueStore &substitutions);
	};

}

#endif //TOOLBOX_TEXTTEMPLATE_H_
