/*
 * Copyright notice...
 */

#include "ctwm.h"

#include <stdlib.h>
#include <stdio.h>

#include "r_area.h"
#include "r_area_list.h"
#include "util.h"


/**
 * Construct an RArea from given components.
 */
RArea
RAreaNew(int x, int y, int width, int height)
{
	RArea area = { x, y, width, height };
	return area;
}


/**
 * Return a pointer to a static newly constructed RArea.
 *
 * This is a thin wrapper around RAreaNew() that returns a static
 * pointer.  This is used in places that need to take RArea pointers, but
 * we don't want to futz with making local intermediate vars.  Currently
 * exclusively used inline in RAreaListNew() calls.
 */
RArea *
RAreaNewStatic(int x, int y, int width, int height)
{
	static RArea area;
	area = RAreaNew(x, y, width, height);
	return &area;
}


/**
 * Return a facially-invalid RArea.
 *
 * This is used in places that need a sentinel value.
 */
RArea
RAreaInvalid()
{
	RArea area = { -1, -1, -1, -1 };
	return area;
}


/**
 * Is an RArea facially valid?
 *
 * Mostly used to check against sentinel values in places that may or may
 * not have a real value to work with.
 */
int
RAreaIsValid(RArea *self)
{
	return self->width >= 0 && self->height >= 0;
}


/**
 * Return the right edge of an RArea.
 */
int
RAreaX2(RArea *self)
{
	return self->x + self->width - 1;
}


/**
 * Return the bottom edge of an RArea.
 */
int
RAreaY2(RArea *self)
{
	return self->y + self->height - 1;
}


/**
 * Return the area of an RArea.
 */
int
RAreaArea(RArea *self)
{
	return self->width * self->height;
}


/**
 * Return an RArea describing the intersection of two RArea's.
 */
RArea
RAreaIntersect(RArea *self, RArea *other)
{
	// Do they even intersect?
	if(RAreaIsIntersect(self, other)) {
		int x1, x2, y1, y2;

		x1 = max(other->x, self->x);
		x2 = min(RAreaX2(other), RAreaX2(self));

		y1 = max(other->y, self->y);
		y2 = min(RAreaY2(other), RAreaY2(self));

		return RAreaNew(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
	}

	// Nope, so nothing
	return RAreaInvalid();
}


/**
 * Do two areas intersect?
 */
int
RAreaIsIntersect(RArea *self, RArea *other)
{
	// [other][self]
	if(RAreaX2(other) < self->x) {
		return 0;
	}

	// [self][other]
	if(other->x > RAreaX2(self)) {
		return 0;
	}

	// [other]
	// [self]
	if(RAreaY2(other) < self->y) {
		return 0;
	}

	// [self]
	// [other]
	if(other->y > RAreaY2(self)) {
		return 0;
	}

	return 1;
}


/**
 * Is a given coordinate inside a RArea?
 */
int
RAreaContainsXY(RArea *self, int x, int y)
{
	return x >= self->x && x <= RAreaX2(self)
	       && y >= self->y && y <= RAreaY2(self);
}


RAreaList *RAreaHorizontalUnion(RArea *self, RArea *other)
{
	// [other]|[self] (perhaps common lines, but areas disjointed)
	if(RAreaX2(other) < self->x - 1) {
		return NULL;
	}

	// [self]|[other] (perhaps common lines, but areas disjointed)
	if(other->x > RAreaX2(self) + 1) {
		return NULL;
	}

	// No lines in common
	// [other] or [self]
	// [self]     [other]
	if(RAreaY2(other) < self->y || other->y > RAreaY2(self)) {
		// Special case where 2 areas with same width can be join vertically
		if(self->width == other->width && self->x == other->x) {
			// [other]
			// [self-]
			if(RAreaY2(other) + 1 == self->y)
				return RAreaListNew(
				               1,
				               RAreaNewStatic(self->x, other->y,
				                              self->width, self->height + other->height),
				               NULL);

			// [self-]
			// [other]
			if(RAreaY2(self) + 1 == other->y)
				return RAreaListNew(
				               1,
				               RAreaNewStatic(self->x, self->y,
				                              self->width, self->height + other->height),
				               NULL);
		}
		return NULL;
	}

	// At least one line in common
	{
		int min_x = min(self->x, other->x);   // most left point
		int max_x = max(RAreaX2(self), RAreaX2(other)); // most right point
		int max_width = max_x - min_x + 1;

		RAreaList *res = RAreaListNew(3, NULL);

		RArea *low, *hi;
		if(self->y < other->y) {
			low = self;
			hi = other;
		}
		else {
			low = other;
			hi = self;
		}

		//     [   ]    [   ]            [   ]    [   ]
		// [hi][low] or [low][hi] or [hi][low] or [low][hi]
		//     [   ]         [  ]        [   ]         [  ]

		if(hi->y != low->y)
			RAreaListAdd(res, RAreaNewStatic(low->x, low->y,
			                                 low->width, hi->y - low->y));

		RAreaListAdd(res,
		             RAreaNewStatic(min_x, hi->y,
		                            max_width,
		                            min(RAreaY2(low), RAreaY2(hi)) - max(low->y, hi->y) + 1));

		if(RAreaY2(low) != RAreaY2(hi)) {
			//     [   ]    [   ]
			// [hi][low] or [low][hi]
			//     [   ]    [   ]
			if(RAreaY2(hi) < RAreaY2(low))
				RAreaListAdd(res,
				             RAreaNewStatic(low->x, RAreaY2(hi) + 1,
				                            low->width, RAreaY2(low) - RAreaY2(hi)));
			//     [   ]    [   ]
			// [hi][low] or [low][hi]
			// [  ]              [  ]
			else
				RAreaListAdd(res,
				             RAreaNewStatic(hi->x, RAreaY2(low) + 1,
				                            hi->width, RAreaY2(hi) - RAreaY2(low)));
		}

		return res;
	}
}

RAreaList *RAreaVerticalUnion(RArea *self, RArea *other)
{
	// [other]
	// ------- (perhaps common columns, but areas disjointed)
	// [self]
	if(RAreaY2(other) < self->y - 1) {
		return NULL;
	}

	// [self]
	// ------- (perhaps common columns, but areas disjointed)
	// [other]
	if(other->y > RAreaY2(self) + 1) {
		return NULL;
	}

	// No columns in common
	// [other][self] or [self][other]
	if(RAreaX2(other) < self->x || other->x > RAreaX2(self)) {
		// Special case where 2 areas with same height can be join horizontally
		if(self->height == other->height && self->y == other->y) {
			// [other][self]
			if(RAreaX2(other) + 1 == self->x)
				return RAreaListNew(
				               1,
				               RAreaNewStatic(other->x, self->y,
				                              self->width + other->width, self->height),
				               NULL);

			// [self][other]
			if(RAreaX2(self) + 1 == other->x)
				return RAreaListNew(
				               1,
				               RAreaNewStatic(self->x, self->y,
				                              self->width + other->width, self->height),
				               NULL);
		}
		return NULL;
	}

	{
		int min_y = min(self->y, other->y);   // top point
		int max_y = max(RAreaY2(self), RAreaY2(other)); // bottom point
		int max_height = max_y - min_y + 1;

		RAreaList *res = RAreaListNew(3, NULL);

		RArea *left, *right;
		if(self->x < other->x) {
			left = self;
			right = other;
		}
		else {
			left = other;
			right = self;
		}

		// [--left--] or  [right]  or    [right] or [left]
		//  [right]     [--left--]    [left]          [right]

		if(right->x != left->x)
			RAreaListAdd(res,
			             RAreaNewStatic(left->x, left->y,
			                            right->x - left->x, left->height));

		RAreaListAdd(res,
		             RAreaNewStatic(right->x, min_y,
		                            min(RAreaX2(left), RAreaX2(right)) - max(left->x, right->x) + 1,
		                            max_height));

		if(RAreaX2(left) != RAreaX2(right)) {
			// [--left--] or  [right]
			//  [right]     [--left--]
			if(RAreaX2(right) < RAreaX2(left))
				RAreaListAdd(res,
				             RAreaNewStatic(RAreaX2(right) + 1, left->y,
				                            RAreaX2(left) - RAreaX2(right), left->height));
			//     [right] or [left]
			//  [left]          [right]
			else
				RAreaListAdd(res,
				             RAreaNewStatic(RAreaX2(left) + 1, right->y,
				                            RAreaX2(right) - RAreaX2(left), right->height));
		}

		return res;
	}
}


/**
 * Pretty-print an RArea.
 *
 * Used for dev/debug.
 */
void
RAreaPrint(RArea *self)
{
	fprintf(stderr, "[x=%d y=%d w=%d h=%d]", self->x, self->y, self->width,
	        self->height);
}
