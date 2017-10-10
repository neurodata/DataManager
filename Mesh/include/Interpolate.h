/*
Copyright (c) 2015, Michael Kazhdan
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of
conditions and the following disclaimer. Redistributions in binary form must reproduce
the above copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the distribution.

Neither the name of the Johns Hopkins University nor the names of its contributors
may be used to endorse or promote products derived from this software without specific
prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE.
*/

template <class Real>
Real LinearInterpolant(Real x1, Real x2, Real isoValue) {
    return (isoValue - x1) / (x2 - x1);
}
template <class Real>
Real QuadraticInterpolant(Real x0, Real x1, Real x2, Real x3, Real isoValue) {
    // Adjust so that we are looking for a zero-crossing
    x0 -= isoValue, x1 -= isoValue, x2 -= isoValue, x3 -= isoValue;
    // Estimate the derivatives at x1 and x2
    Real dx1 = (x2 - x0) / 2.f, dx2 = (x3 - x1) / 2.f;
    // Solve for the quadratic polynomial:
    //		P(x) = a x^2 + b x + c
    // such that:
    //		P(0) = x1 , P(1) = x2 , and minimizing || P'(0) - dx1 ||^2 + || P'(1) - dx2 ||^2
    //	=>  c = x1 , a = x2 - x1 - b , and minimizing || b - dx1 ||^2 + || 2*a + b - dx2 ||^2
    //	=>  c = x1 , a = x2 - x1 - b , and minimizing || b - dx1 ||^2 + || 2*x2 - 2*x1 - b - dx2 ||^2
    //	=>  c = x1 , a = x2 - x1 - b , and minimizing || b - dx1 ||^2 + || b - ( 2*x2 - 2*x1 - dx2 ) ||^2
    //	=>  c = x1 , b = ( 2*x2 - 2*x1 - dx2 + dx1 ) / 2 , a = x2 - x1 - b
    //	=>  c = x1 , b = ( x2 - x1 ) - ( dx2 - dx1 ) / 2 , a = ( dx2 - dx1 ) / 2

    double a = (dx2 - dx1) / 2.f, b = (dx1 - dx2) / 2.f + x2 - x1, c = x1;
    if (!a) {
        // Solve b * x + c = 0
        return (Real)(-c / b);
    } else {
        // Solve a x^2 + b x + c = 0
        b /= a, c /= a;
        double disc = b * b - 4. * c;
        if (disc < 0) fprintf(stderr, "[ERROR] Negative discriminant: %g\n", disc), exit(0);
        disc = sqrt(disc);
        double r1 = (-b - disc) / 2., r2 = (-b + disc) / 2.;
        if (r2 < 0 || r1 > 1) fprintf(stderr, "[ERROR] Roots out of bounds: %g %g\n", r1, r2), exit(0);
        if (r2 > 1)
            return (Real)r1;
        else
            return (Real)r2;
    }
}