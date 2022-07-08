#ifndef __PHYS_MATH_H
#define __PHYS_MATH_H

struct vector_3
{
	float x, y, z;

	// Vector addition
	vector_3  operator+  (vector_3 const& other) const;
	// Scalar multiplication
	vector_3  operator*  (float const factor) const;
	// Dot product
	float     operator*  (vector_3 const& other) const;

	// vector1 = vector1 + vector2
	vector_3& operator+= (vector_3 const& other);
	// vector = vector * factor
	vector_3& operator*= (float const factor);

	// Vector = Vector / |Vector|
	vector_3& normalize();
};

#endif
