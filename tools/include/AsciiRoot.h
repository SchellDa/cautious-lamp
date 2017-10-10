#ifndef __Alibava_AsciiRoot_h__
#define __Alibava_AsciiRoot_h__

#define CMMD_BLOCKS 2
#include <vector>
#include "Data.h"
#include "Hit.h"
#include <ctime>
#include <TH1.h>
#include <TH2.h>

/** This is the class that reads the data files
 */
class AsciiRoot
{
    public:
        typedef std::vector<std::string> XtraValues;
        typedef std::vector< Hit> HitList;
	enum BlockType { NewFile=0, StartOfRun, DataBlock, CheckPoint, EndOfRun };
    private:
        std::ifstream *ifile;
        unsigned int data_start;
        int _type;
        time_t _t0;
        int _npoints;
        int _from;
        int _to;
        int _step;
        int _nevts;
        int _nchan; // current number of channels
        double _seedcut;
        double _neighcut;
        double _ped[256];
        double _noise[256];
        double _signal[256];
        double _sn[256];
        double _cmmd[CMMD_BLOCKS];
        double _cnoise[CMMD_BLOCKS];
        double _gain[256];
        double _average_gain;
        bool   _mask[256];
        bool   _user_mask[256];
	double _maxnoise;
	double _minnoise;
	int _maskrange1;
	int _maskrange2;
        HitList _hits;

        EventData _data;

	int _version;
	XtraValues _xtra; // extra values from header
       	unsigned short _header[2][16];

	double noisevalue;

    protected:
        void reset_data();



    public:
	AsciiRoot() {};
        AsciiRoot(const char *nam, const char *pedfile=0, const char *gainfile=0);
        virtual ~AsciiRoot();

        bool valid() const
        {
            return (ifile!=0);
        }

        void open(const char *name);
        void close();
        void rewind();
        int read_event();
	virtual void check_point(int, const char *) {};
        virtual void new_file(int, const char *) {}
        virtual void start_of_run(int, const char *) {}
        virtual void end_of_run(int, const char *) {}
        virtual void new_data_block(int, const char *) {};

        int nchan() const { return _nchan; }

        int type() const
        {
            return _type;
        }
	
        char *date() const
        {
            return ctime(&_t0);
        }
	
        double ped(int i) const
        {
            return _ped[i];
        }
        double ped_cal(int i) const
        {
            return _ped[i]/_gain[i];
        }
        double noise(int i) const
        {
            return _noise[i];
        }
        double noise_cal(int i) const
        {
            return _noise[i]/_gain[i];
        }
        bool mask(int i) const
        {
            return _mask[i];
        }
        double signal(int i) const
        {
	  return _signal[i];
        }
        double signal_cal(int i) const
        {
	  //return _signal[i*2-127*(i/64)+126*(i/128)]/_gain[i*2-127*(i/64)+126*(i/128)];
	  return _signal[i]/_gain[i];
        }
        
        double sn(int i) const
        {
	  //printf("request %d got %d\n",i,(i*2-127*(i/64)+126*(i/128)));
	  //return _sn[i*2-127*(i/64)+126*(i/128)];
	  return _sn[i];
        }
        
        double get_cmmd(int i) const
        {
            return _cmmd[i];
        }
        
        double get_cnoise(int i) const
        {
            return _cnoise[i];
        }
        
        unsigned short data(int i) const
        {
	  return _data.data[i];
        }
        double value() const
        {
            return _data.value;
        }
        double clock() const;
	double time() const;
       double temp() const;
        int npts() const
        {
            return _npoints;
        }
        int from() const
        {
            return _from;
        }
        int to() const
        {
            return _to;
        }
        int step() const
        {
            return _step;
        }
        int nevts() const
        {
            return _nevts;
        }

        void add_hit(const Hit &h)
        {
            _hits.push_back(h);
        }
        HitList::iterator begin()
        {
            return _hits.begin();
        }
        HitList::iterator end()
        {
            return _hits.end();
        }
        int nhits() const
        {
            return _hits.size();
        }
        bool empty() const
        {
            return _hits.empty();
        }
        const Hit &hit(int i) const
        {
            return _hits[i];
        }
        void clear()
        {
            _hits.clear();
        }
        double get_gain(int i) const 
        {
            return _gain[i];
        }
        
        void set_gain(int i, double g)
        {
            _gain[i]=g;
        }
        double gain() const 
        {
            return _average_gain;
        }

        double seed_cut() const 
        {
            return _seedcut;
        }
        double neigh_cut() const
        {
            return _neighcut;
        }
        void set_cuts(double s, double n)
        {
            _seedcut = s;
            _neighcut = n;
        }
        void set_noise_cuts(double upper, double lower)
        {
            _maxnoise = upper;
            _minnoise = lower;
        }
	double max_noise(){
	  return _maxnoise;
	}
	double min_noise(){
	  return _minnoise;
	}

        void set_alt_type(int type)
        {
            _type = type;
        }

        void add_mask_channel(int ch)
        {
            _mask[ch] = true;
            _user_mask[ch] = true;
        }

	//	unsigned short get_header(int ichip, int ibit) { return _header[ichip][ibit];}

	XtraValues get_xtra()
        {
		if (_xtra.size()==0)
			_xtra.push_back("");
		return _xtra;
	}

	int get_version()
	{
	  return _version;
	}

        void set_mask_range(const char *cmask);

        TH1 *show_pedestals();
        TH1 *show_noise();
        TH1 *show_noise_calib();
        TH1 *show_mask();
        TH1 *show_gain();
        TH2 *compute_pedestals(int start=0, int mxevts=-1, bool do_cmmd=true, char * name = "dummy", int pol=1);
        void compute_pedestals_fast(int mxevts = -1, double ped_weight=0.01, double noise_weight=0.001);
        
        void process_event(bool do_cmmd=true);
        void find_clusters(double polarity=0., int ichip=-1);
        void save_pedestals(const char *fnam);
        void load_pedestals(const char *fnam);
        void save_gain(const char *fnam);
        void load_gain(const char *fnam);
        void spy_time(int nevt=1);
        void spy_data(bool with_signal=false, int nevt=1, double polarity=0.,int first_strip=0, int last_strip=255);
        void common_mode();
        void common_mode_group();
	TH1*  show_noise_plots(bool with_signal, double polarity, bool issignalrun);

        int nxtra() const { return _xtra.size(); }
        const std::string xtra(int i) const { return _xtra[i]; }
        void add_xtra(const std::string &x) { _xtra.push_back(x); }
        void add_xtra(const char *x) { _xtra.push_back(x); }


};


#endif
