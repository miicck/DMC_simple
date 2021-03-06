/*

    XDMC
    Copyright (C) Michael Hutcheon (email mjh261@cam.ac.uk)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    For a copy of the GNU General Public License see <https://www.gnu.org/licenses/>.

*/

#include "catch.h"
#include "dmc_math.h"
#include "params.h"

int sign(double val)
{
    // Returns the sign of non-zero values
    // or 0 for exactly 0 values
    return (0 < val) - (val < 0);
}

double coulomb(double q1, double q2, double r)
{
    // The coulomb interation used throughout
    // the code, including any softening
    return q1*q2/(r+params::coulomb_softening);
}

unsigned factorial(unsigned n)
{
    // Returns n!
    if (n == 0) return 1;
    return n * factorial(n-1);
}

TEST_CASE("Basic math functions", "[math]")
{
    // Test the coulomb interaction
    REQUIRE(coulomb(1,1,1)  == 1.0 );
    REQUIRE(coulomb(-1,1,1) == -1.0);
    REQUIRE(coulomb(1,-1,1) == -1.0);
    REQUIRE(coulomb(1,1,2)  == 0.5 );
    REQUIRE(coulomb(-1,1,2) == -0.5);
    REQUIRE(coulomb(1,-1,2) == -0.5);

    // Test the sign function
    REQUIRE(sign(0)  == 0 );
    REQUIRE(sign(1)  == 1 );
    REQUIRE(sign(-1) == -1);

    // Test the factiorial function
    REQUIRE(factorial(0) == 1  );
    REQUIRE(factorial(1) == 1  );
    REQUIRE(factorial(2) == 2  );
    REQUIRE(factorial(5) == 120);
}

