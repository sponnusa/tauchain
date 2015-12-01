#ifndef __MARPA_TAU_H__
#define __MARPA_TAU_H__

#include "prover.h"
#include "cli.h"
#include <istream>
#include "misc.h"

ParsingResult parse_natural3(qdb &kb, qdb &q, std::istream &f, string base = "@default");

#endif
