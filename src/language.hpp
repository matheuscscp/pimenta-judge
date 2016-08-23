#ifndef LANGUAGE_H
#define LANGUAGE_H

#include "json.hpp"

namespace Language {

JSON list(int probid);
JSON settings(const JSON& attempt);

} // namespace Language

#endif
