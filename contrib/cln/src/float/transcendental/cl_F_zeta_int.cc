// zeta().

// General includes.
#include "base/cl_sysdep.h"

// Specification.
#include "cln/float.h"


// Implementation.

#include "float/cl_F.h"
#include "float/transcendental/cl_F_tran.h"
#include "float/lfloat/cl_LF.h"

namespace cln {

const cl_F zeta (int s, const cl_F& y)
{
	floattypecase(y
	,	return cl_LF_to_SF(zeta(s,LF_minlen));
	,	return cl_LF_to_FF(zeta(s,LF_minlen));
	,	return cl_LF_to_DF(zeta(s,LF_minlen));
	,	return zeta(s,TheLfloat(y)->len);
	);
}

}  // namespace cln
