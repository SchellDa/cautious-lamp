#ifndef __HIT_H__
#define __HIT_H__

/**
 * A class representing a hit
 */
class Hit
{
    private:
        double _center;
        int _left;
        int _right;
        double _sig;
        double _sig_cal;
        
        void cpy(const Hit &h);
    public:
        Hit(double c=0, int l=0, int r=0, double s=0, double s_c=0);
        Hit(const Hit &h);
        ~Hit();
        
        Hit &operator=(const Hit &h);
        
        double center() const { return _center; }
        int left() const { return _left; }
        int right() const { return _right; }
        double signal() const { return _sig; }
        double signal_cal() const { return _sig_cal; }
        int width() const { return _right - _left + 1; }
};


#endif /*__HIT_H__*/
