// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// add headers that you want to pre-compile here
#include "framework.h"
#include "SDK/Includes.h"
using namespace SDK;
#include "Erbium/Public/Utils.h"
#include <numeric>
#include <algorithm>
#include "libcurl/curl.h"

inline UEAllocatedString iso8601() {
	time_t now;
	time(&now);
	char buf[sizeof "2011-10-08T07:07:09Z"];
	tm* t = new tm();
	gmtime_s(t, &now);
	strftime(buf, sizeof buf, "%FT%TZ", t);
	return UEAllocatedString(buf);
}

#endif //PCH_H
