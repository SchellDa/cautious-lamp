#include "Hit.h"

Hit::Hit(int c, int l, int r, double s, double s_c) :
  _center(c), _left(l), _right(r), _sig(s), _sig_cal(s_c)
{
}

Hit::Hit(const Hit &h)
{
    cpy(h);
}
Hit::~Hit()
{
}

void Hit::cpy(const Hit &h)
{
    _center = h._center;
    _left = h._left;
    _right = h._right;
    _sig = h._sig;
    _sig_cal = h._sig_cal;
}
Hit &Hit::operator=(const Hit &h)
{
    if (&h!=this)
        cpy(h);

    return *this;
}
