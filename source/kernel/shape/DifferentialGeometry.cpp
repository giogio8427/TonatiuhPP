#include "DifferentialGeometry.h"


DifferentialGeometry::DifferentialGeometry():
    u(0.), v(0.), shape(0)
{

}

DifferentialGeometry::DifferentialGeometry(
    const Vector3D& point,
    double u, double v,
    const Vector3D& dpdu, const Vector3D& dpdv,
    const Vector3D& normal,
    const ShapeRT* sh, bool isFront):
    point(point),
    u(u), v(v),
    dpdu(dpdu), dpdv(dpdv),
    normal(normal),
    shape(sh), isFront(isFront)
{

}
