#include <math.h>
#include "Phys_Math.h"


vector_3 vector_3::operator+ (vector_3 const& other) const
{
	return {
		x + other.x,
		y + other.y,
		z + other.z
	};
}


vector_3 vector_3::operator* (float const factor) const
{
	return {
		x * factor,
		y * factor,
		z * factor
	};
}


vector_3& vector_3::operator+= (vector_3 const& other)
{
	*this = *this + other;
	return *this;
}


vector_3& vector_3::operator*= (float const factor)
{
	*this = *this * factor;
	return *this;
}


float vector_3::operator* (vector_3 const& other) const
{
	return x * other.x + y * other.y + z * other.z;
}


vector_3& vector_3::normalize()
{
	const float length = *this * *this;
	const float factor = length == 0 ? 0 : (1 /sqrt(length));

	*this *= factor;
	return *this;
}
