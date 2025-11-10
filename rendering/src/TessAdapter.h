#ifndef TESS_ADAPTER_H
#define TESS_ADAPTER_H

#include "DataTypes.h"

/**
 * @file TessAdapter.h
 * @brief Provides the "glue" code to make earcut.hpp compatible.
 *
 * This file provides template specializations for the mapbox::util::nth
 * struct. This is the mechanism `earcut.hpp` uses to find the X and Y
 * coordinates of a vertex.
 *
 */

namespace mapbox
{
    namespace util
    {

        /**
         * @brief Specialization of `nth` for the 0th element (X-coordinate).
         * This tells earcut that to get the 'X' value of a `Point`,
         * it should access `p.x`.
         */
        template <>
        struct nth<0, Point>
        {
            inline static float get(const Point &p)
            {
                return p.x;
            };
        };

        /**
         * @brief Specialization of `nth` for the 1st element (Y-coordinate).
         * This tells earcut that to get the 'Y' value of a `Point`,
         * it should access `p.y`.
         */
        template <>
        struct nth<1, Point>
        {
            inline static float get(const Point &p)
            {
                return p.y;
            };
        };

    }
}

#endif