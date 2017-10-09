#include <iostream>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <cmath>
#include <sstream>
#include <csignal>
#include <vector>
#include <cerrno>
#include <climits>
#include <TROOT.h>
#include <TCanvas.h>
#include <TProfile2D.h>
#include <TF1.h>
#include <TH1.h>
#include <TFile.h>
#include <TMath.h>
#include <TGraph.h>

#include "AsciiRoot.h"
#include "utils.h"
//#include "Tracer.h"

using namespace std;

bool _A_do_run = true;
void _A_got_intr(int)
{
    _A_do_run = false;
}

// decodes the header and returns a vector with the integers found
std::vector<int> decode_header(const std::string &h, AsciiRoot::XtraValues &xtra)
{
    std::vector<int> vout;
    std::istringstream istr(h);
    //char *endptr, *str;
    char *endptr;
    char buf[256];
    long val;

    xtra.clear();

    while (istr)
    {
        istr.getline(buf, sizeof(buf), ';');
        if (!istr)
            break;

        errno = 0;
        val = strtol(buf, &endptr, 0);

        if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN)) || (errno != 0 && val == 0))
        {
            std::string sval(buf), sout;
            sout = trim_str(sval);
            if (!sout.empty())
                xtra.push_back( sout );
        }
        else if ( endptr == buf || *endptr != '\0' )
        {
            std::string sval(buf), sout;
            sout = trim_str(sval);
            if (!sout.empty())
                xtra.push_back( sout );
        }
        else
        {
            vout.push_back(atoi(buf) );
        }
    }
    return vout;
}


AsciiRoot::AsciiRoot(const char *nam, const char *pedfile, const char *gainfile) :
  _seedcut(5.), _neighcut(2.5), _average_gain(1.), _maxnoise(5.), _minnoise(1.), _maskrange1(-1), _maskrange2(-1), _version(2)
{
    int i;
    for (i=0;i<256;i++)
    {
        _ped[i] = 0.;
        _gain[i] = 1.;
        _noise[i] = 1.;
        _mask[i] = false;
	_user_mask[i] = false;
    }
    if (nam)
        open(nam);
    
    if (pedfile)
        load_pedestals(pedfile);
    
    if (gainfile)
        load_gain(gainfile);
}
AsciiRoot::~AsciiRoot()
{
    if (ifile)
    {
        ifile->close();
    }
}
void AsciiRoot::open(const char *name)
{
    ifile = new std::ifstream(name);
    if (!(*ifile))
    {
        std::cout << "Could not open " << name << std::endl;
        delete ifile;
        ifile = 0;
	return;
    }
    std::string header;
    unsigned int ic, lheader;
    char c;
    
    //ifile->read((char *)&_t0, sizeof(time_t));
    ifile->read((char *)&_t0, sizeof(double));
    std::cout << "64b: " << ctime(&_t0) << std::endl;
    ifile->read((char *)&_type, sizeof(int));
    std::cout << "type_ " << _type << std::endl;
    if ( _type > 15 || _type == 0)
    {
        ifile->seekg(0, std::ios::beg);
        ifile->read((char *)&_t0, sizeof(int));
        ifile->read((char *)&_type, sizeof(int));
        std::cout << "32b: " << ctime(&_t0) << std::endl;
    }
    //cout<<_t0<<endl;
    
    
    // ifile->read((char *)&_t0, sizeof(time_t));

    // ifile->read((char *)&_t0, sizeof(int));
    //     cout<<_t0<<endl;

    // ifile->read((char *)&_type, sizeof(int));
    //     cout<<_type<<endl;
    // if(_type>5 || _type<1)
    //   ifile->read((char *)&_type, sizeof(int));
    //     cout<<_type<<endl;
    

    ifile->read((char *)&lheader, sizeof(unsigned int));
    cout<<lheader<<endl;

    //for (ic=0; ic<lheader; ic++)
    for (ic=0; ic<80; ic++)
    {
        ifile->read(&c, sizeof(char));
        header.append(1, c);
    }
    header = trim_str(header);
    std::cout <<" header: " << header << std::endl;

   if (header[0]!='V' && header[0]!='v')
    {
        _version = 0;
    }
    else
    {
        _version = int(header[1]-'0');
        header = header.substr(5);
	std::cout <<"NEW FIRMWARE!!! "<< _version << std::endl;
    }

        std::cout << "type: " << _type << " header: " << header << std::endl;
   std::vector<int> param = decode_header(header, _xtra);
    ifile->read((char *)_ped, 256*sizeof(double));
    ifile->read((char *)_noise, 256*sizeof(double));
       std::cout << "noise: " << _noise[0] << " ped " << _ped[0] << std::endl;
       std::cout << "noise: " << _noise[255] << " ped " << _ped[255] << std::endl;
   switch (_type)
    {
    case 1: // calibration
    case 2: // laser sync
      _npoints = param[0];
      _from = param[1];
      _to = param[2];
      _step = param[3];
      break;
    case 3: // laser run
    case 4: // source run
    case 5: // pedestal run
      if (param.empty())
	_nevts = 100000;
      else
	_nevts = param[0];
      _npoints = _from = _to = _step = 0;
      break;
    default:
      break;
     }
    data_start = ifile->tellg();
}

void AsciiRoot::rewind()
{
    if (ifile)
    {
        ifile->clear();
        ifile->seekg(data_start, std::ios::beg);
    }
}

void AsciiRoot::close()
{
    if (ifile)
    {
        ifile->close();
        delete ifile;
        ifile = 0;
    }
}

void AsciiRoot::reset_data()
{
    memset(&_data, 0, sizeof(_data));
}


int AsciiRoot::read_event()
{
    if (ifile)
    {
        unsigned int header, size, user=0, code=0;
        char *block_data=0;
        if (_version)
        {
            do
            {
                do
                {
                    ifile->read((char *)&header, sizeof(unsigned int));
                    if (ifile->bad() || ifile->eof())
                        return -1;

                    code = (header>>16) & 0xFFFF;
                } while ( code != 0xcafe );

                code = header & 0x0fff;
                user = header & 0x1000;
		//std::cout<<"code: " << code<<std::endl;
		//std::cout<<"user: " << user<<std::endl;

                switch (code)
                {
                    case NewFile:
		      std::cout<<"New file" << std::endl;
                        ifile->read((char *)&size, sizeof(unsigned int));
                        block_data = new char[size];
                        ifile->read(block_data, size);
                        new_file(size, block_data);
                        break;
                    case StartOfRun:
		      std::cout<<"Start of run" << std::endl;
                        ifile->read((char *)&size, sizeof(unsigned int));
                        block_data = new char[size];
                        ifile->read(block_data, size);
                        start_of_run(size, block_data);
                        break;
                    case DataBlock:
		      //std::cout<<"DataBlock" << std::endl;
                        ifile->read((char *)&size, sizeof(unsigned int));
                        if (user)
                        {
                            reset_data();
                            block_data = new char[size];
                            ifile->read(block_data, size);
                            new_data_block(size, block_data);
                        }
                        else
                        {
                            if ( _version == 1 )
                            {
                                ifile->read((char *)&_data, sizeof(EventData));
                                for (int ii=0; ii<2; ii++)
                                    memset(_header[ii], 0, 16*sizeof(unsigned short));

                            }
                            else
                            {
                                ifile->read((char *)&_data.value, sizeof(double));
                                if (_version > 2){
				  //std::cout<<"Version 3 clock data" << std::endl;
                                    ifile->read((char *)&_data.clock, sizeof(unsigned int));
				}
                                ifile->read((char *)&_data.time, sizeof(unsigned int));
                                ifile->read((char *)&_data.temp, sizeof(unsigned short));
                                for (int ii=0; ii<2; ii++)
                                {
				  ifile->read((char *)&_header[ii], 16*sizeof(unsigned short)); // test
				  //ifile->read((char *)_header[ii], 16*sizeof(unsigned short));//???
                                    ifile->read((char *)&_data.data[ii*128], 128*sizeof(unsigned short));
                                }
                            }
                        }

                        break;


                    case CheckPoint:
                        ifile->read((char *)&size, sizeof(unsigned int));
                        block_data = new char[size];
                        ifile->read(block_data, size);
                        check_point(size, block_data);
                        break;
                    case EndOfRun:
                        ifile->read((char *)&size, sizeof(unsigned int));
                        block_data = new char[size];
                        ifile->read(block_data, size);
                        end_of_run(size, block_data);
                        break;
                    default:
                        std::cout << "Unknown block data type: " << std::hex << header << " - " << code << std::dec << std::endl;
                }
                if (block_data)
                {
                    delete [] block_data;
                    block_data = 0;
                }

            } while ( code != DataBlock && !(ifile->bad() || ifile->eof()) );
        }
        else
        {
            ifile->read((char *)&_data, sizeof(EventData));
            for (int ii=0; ii<2; ii++)
                memset(_header[ii], 0, 16*sizeof(unsigned short));
        }

        if (ifile->eof())
        {
            std::cout << "End of file" << std::endl;
            return -1;
        }
        else if (ifile->bad())
        {
            std::cout << "Problems with data file" << std::endl;
            return -1;
        }
        else
        {
            //process_event();
            return 0;
        }
    }
    else
        return -1;
}

double AsciiRoot::clock() const
{
  return _data.clock;
}

double AsciiRoot::time() const
{

    unsigned short fpart = _data.time & 0xffff;
    short ipart = (_data.time & 0xffff0000)>>16;
    if (ipart<0)
        fpart *= -1;
    double tt = 100.*(ipart + (fpart/65535.));
    //std::cout << _data.time << " -> " << tt << " ( " << ipart << "," << fpart << ")" << std::endl;
    return tt;
}

double AsciiRoot::temp() const
{
  double t = 0.12*_data.temp - 39.8;
  //std::cout << _data.temp << " -> " << t <<  std::endl;
  return t;
}

TH1 *AsciiRoot::show_pedestals()
{
    int ic;
    TH1 *hst = new TH1D("hPed","Pedestals",256,-0.5, 255.5);
    hst->SetYTitle("ADCs");
    hst->SetXTitle("Channel no.");
    for (ic=0; ic<256; ic++)
        hst->SetBinContent(ic+1, _ped[ic]);

    return hst;
}

TH1 *AsciiRoot::show_mask()
{
    int ic;
    TH1 *hst = new TH1D("hMask","Mask",256,-0.5, 255.5);
    hst->SetYTitle("Flag");
    hst->SetXTitle("Channel no.");
    for (ic=0; ic<256; ic++)
        hst->SetBinContent(ic+1, _mask[ic]);

    return hst;
}

TH1 *AsciiRoot::show_noise()
{
    int ic;
    TH1 *hst = new TH1D("hNoise","Noise",256,-0.5, 255.5);
    hst->SetXTitle("Channel no.");
    hst->SetYTitle("ADCs");
    for (ic=0; ic<256; ic++)
      hst->SetBinContent(ic+1, noise(ic));
    
    return hst;
}

TH1 *AsciiRoot::show_gain()
{
    int ic;
    TH1 *hst = new TH1D("hGain","Gain",256,-0.5, 255.5);
    hst->SetXTitle("Channel no.");
    hst->SetYTitle("e/ADC");
    for (ic=0; ic<256; ic++)
      hst->SetBinContent(ic+1, 1./get_gain(ic));
    
    return hst;
}

TH1 *AsciiRoot::show_noise_calib()
{
    int ic;
    TH1 *hst = new TH1D("hNoise","Noise",256,-0.5, 255.5);
    hst->SetXTitle("Channel no.");
    if (gain()==1)
    {
        hst->SetYTitle("ADCs");
	for (ic=0; ic<256; ic++)
	  hst->SetBinContent(ic+1, noise(ic));
    }
    else
    {
        hst->SetYTitle("e^{-} ENC");
	for (ic=0; ic<256; ic++)
	  if(this->get_gain(ic)!=1)
	    hst->SetBinContent(ic+1, noise_cal(ic));
    }
    
    return hst;
}

void AsciiRoot::compute_pedestals_fast(int mxevts, double wped, double wnoise)
{
    int i, ievt;
    
    if (!ifile)
        return;

    if (mxevts<0)
        mxevts = 100000000;

    for (i=0;i<256;i++)
        _ped[i] = _noise[i] = 0.;
    
    std::ifstream::pos_type here = ifile->tellg();
    std::cout << "Computing fast pedestas..." << std::endl;
    for (ievt=0; read_event()==0 && ievt<mxevts; ievt++)
    {
        if (!(ievt%100))
        {
            std::cout << "\revent " << std::setw(10) << ievt << std::flush;
        }
        common_mode();
        for (i=0; i<256; i++)
        {
            int ichip = i/128;
            // IF noise is 0, set it arbitrarily to 1.
            if (_noise[i]==0.)
                _noise[i] = 1.;

            if (_ped[i]==0.)
            {
                // If pedestal is not yet computed we assume the current
                // channel value should not be too far
                _ped[i] = _data.data[i];
            }
            else
            {
                // Do the pedestal and noise correction
                double corr;
                double xs;

                _signal[i] = _data.data[i] - _ped[i];
                corr = _signal[i] * wped;
                
                xs = (_signal[i]-_cmmd[ichip])/_noise[i];
                if (corr > 1.)
                    corr = 1.;

                if (corr < -1)
                    corr = -1.;

                _ped[i] += corr;

                if (fabs(xs) < 3.)
                {
                    _noise[i] = _noise[i]*(1.0-wnoise) + xs*xs*wnoise;
                }
            }
        }
    }
    std::cout << "\nDone" << std::endl;
    rewind();    
}



TH2 *AsciiRoot::compute_pedestals(int start, int mxevts, bool do_cmmd, char * name, int polarity)
{
    if (!ifile)
        return 0;

    if (mxevts<0)
        mxevts = 100000000;

    int ievt, ichan;
    TH2 *hst = create_h2("hRaw","Raw data",256, -0.5,255.5,1024, -0.5,1023.5);
    TH2 *hst2 = create_h2("hRaw2","CMS data",256, -0.5,255.5,1024, -0.5,1023.5);
    TFile *f = new TFile(name,"RECREATE");

    TH1 *hCmmd0 = create_h1("hCmmd0","Common Mode vs. Event",mxevts, -0.5,mxevts-0.5);
    TH1 *hCmmd1 = create_h1("hCmmd1","Common Mode vs. Event",mxevts, -0.5,mxevts-0.5);
    TH1 *hCmmd_dist0 = create_h1("hCmmd_dist0","Common Mode Distribution Group 0",551, -550.5,550.5);
    TH1 *hCmmd_dist1 = create_h1("hCmmd_dist1","Common Mode Distribution Group 1",551, -550.5,550.5);


    std::ifstream::pos_type here = ifile->tellg();
    std::cout << "Computing pedestals..." << std::endl;
//     for(int i=0;i<256;i++)
//       std::cout << _user_mask[i] << " ";
//     std::cout<<std::endl;

    // Get the raw data first
    for (ievt=0; read_event()==0 && ievt<start; ievt++)
    {
    }    

    for (ievt=0; read_event()==0 && ievt<mxevts; ievt++)
    {
      for (ichan=0; ichan<256; ichan++)
	hst->Fill(ichan, data(ichan));
      
        if (!(ievt%100))
        {
	  std::cout << "\revent " << std::setw(10) << ievt <<std::flush;
        }
    }



    if(do_cmmd){

      // calculate pedestals and noise from raw data with no cuts
      for (ichan=0; ichan<256; ichan++)
	{
	  TF1 *g = new TF1("g1", "gaus");
	  TH1 *h1 = hst->ProjectionY(Form("_sig-nocms_%d",ichan), ichan+1, ichan+1);
	  g->SetParameters(h1->GetSumOfWeights(), h1->GetMean(), h1->GetRMS());
	  g->SetRange(h1->GetMean()-2.5*h1->GetRMS(), h1->GetMean()+2.5*h1->GetRMS());
	  h1->Fit("g1", "qwr");
	  h1->Write();
	  _ped[ichan] = h1->GetFunction("g1")->GetParameter(1);
	  _noise[ichan] = h1->GetFunction("g1")->GetParameter(2);
	  _mask[ichan] = _user_mask[ichan];
	  delete h1;
	  delete g;
	}


      
      std::cout << "\nFirst pass done" << std::endl;

      rewind();


      // get common mode and subtract from data //2012-06-01 Maske für CommonMode berücksichtigen? Ja wird berücksichtigt!AD
      for (ievt=0; read_event()==0 && ievt<start; ievt++)
	{
	}    
      
      for (ievt=0; read_event()==0 && ievt<mxevts; ievt++)
	{
	  common_mode_group();
	
	  hCmmd0->SetBinContent(ievt,get_cmmd(0));
	  hCmmd1->SetBinContent(ievt,get_cmmd(1));
	  hCmmd_dist0->Fill(get_cmmd(0));
	  hCmmd_dist1->Fill(get_cmmd(1));

	  for (ichan=0; ichan<256; ichan++)
	    hst2->Fill(ichan, data(ichan)-get_cmmd(ichan/128));
	  
	  if (!(ievt%100))
	    {
	      std::cout << "\revent " << std::setw(10) << ievt << std::flush;
	    }
	}

      // calculate pedestals and noise from CMS data and mask bad strips
      for (ichan=0; ichan<256; ichan++)
	{
	  TF1 *g = new TF1("g1", "gaus");
	  TH1 *h1 = hst2->ProjectionY(Form("_sig_cms1_%d",ichan), ichan+1, ichan+1);
	  g->SetParameters(h1->GetSumOfWeights(), h1->GetMean(), h1->GetRMS());
	  g->SetRange(h1->GetMean()-2.5*h1->GetRMS(), h1->GetMean()+2.5*h1->GetRMS());
	  h1->Fit("g1", "qwr+");
	  h1->Write();
	  _ped[ichan] = h1->GetFunction("g1")->GetParameter(1);
	  _noise[ichan] = h1->GetFunction("g1")->GetParameter(2);
	  // here we have the first good mask 
	  _mask[ichan] = (_noise[ichan]>_maxnoise || _noise[ichan]<_minnoise || _user_mask[ichan]);  
	  delete h1;
	  delete g;
	}

      rewind();
      std::cout << "\nSecond pass done" << std::endl;


      // get CM with bad strip mask applied
      for (ievt=0; read_event()==0 && ievt<start; ievt++)
	{
	}    
      
      for (ievt=0; read_event()==0 && ievt<mxevts; ievt++)
	{
	  common_mode_group();
	
	  hCmmd0->SetBinContent(ievt,get_cmmd(0));
	  hCmmd1->SetBinContent(ievt,get_cmmd(1));
	  hCmmd_dist0->Fill(get_cmmd(0));
	  hCmmd_dist1->Fill(get_cmmd(1));

	  for (ichan=0; ichan<256; ichan++)
	    hst2->Fill(ichan, data(ichan)-get_cmmd(ichan/128));
	  
	  if (!(ievt%100))
	    {
	      std::cout << "\revent " << std::setw(10) << ievt << std::flush;
	    }
	}


      // reset mask
      for (ichan=0; ichan<256; ichan++)
	{
	  _mask[ichan] = false;  
	}

      hCmmd0->Write();
      hCmmd1->Write();
      hCmmd_dist0->Write();
      hCmmd_dist1->Write();
      hst = hst2;

    }


    // calculate final pedestals and noise
    for (ichan=0; ichan<256; ichan++)
      {
        TF1 *g = new TF1("g1", "gaus");
        TH1 *h1 = hst->ProjectionY(Form("_sig_%d",ichan), ichan+1, ichan+1);
        g->SetParameters(h1->GetSumOfWeights(), h1->GetMean(), h1->GetRMS());
        g->SetRange(h1->GetMean()-2.5*h1->GetRMS(), h1->GetMean()+2.5*h1->GetRMS());
        h1->Fit("g1", "qwr+");
	h1->Write();
        _ped[ichan] = h1->GetFunction("g1")->GetParameter(1);
        _noise[ichan] = h1->GetFunction("g1")->GetParameter(2);
	_mask[ichan] = (_noise[ichan]>_maxnoise || _noise[ichan]<_minnoise || _user_mask[ichan]);  
        delete h1;
        delete g;
      }
    
    std::cout << "\nDone" << std::endl;
    rewind();
    TH1 * h = this->show_noise();
    h->Write();
    h = this->show_pedestals();
    h->Write();
    h = this->show_mask();
    h->Write();
    this->rewind();
    h = this->show_noise_plots(false, polarity, false);
    h->Write();
    f->Close();
    delete f;

    //this->show_noise()->Draw();
    //this->show_mask()->Draw("SAME");
    
    return hst;
}


/*
///für strixel
void AsciiRoot::find_clusters(double polarity, int ichip)
{
    int i, imax=-1, left, right;
    double mxsig=-1.e20, sg, val;
    bool used[256];
    int chan0=0;
    int chan1=256;
    
    if (ichip>=0 && ichip<2)
    {
        chan0 = ichip*128;
        chan1 = (ichip+1)*128;
    }
    
    
    for (i=0;i<256;i++)
    {
        used[i]= _mask[i] ? true : false;
    }
    clear();

        
    while (true)
    {
        
        //////////////// Find the highest 
      
        imax = -1;
        for (i=chan0; i<chan1; i++)
        {
	  if (used[i] || signal(i)*polarity<0.)
                continue;
            
            if (fabs(sn(i))>_seedcut)
            {
                val = fabs(signal(i));
                if (mxsig<val)
                {
                    mxsig = val;
                    imax = i;
                }
            }
        }

        if (imax<0)
            break;

        sg = signal(imax);
        used[imax]=true;
        // Now look at neighbours
        // first to the left
        left = imax;
        for (i=imax-2;i>=0;i-=2)
        {
	  if ( signal(i)*polarity<0.)
	    break;
	  
	  if ( fabs(sn(i)) > _neighcut )
            {
	      used[i] = true;
	      sg += signal(i);
	      left = i;
	      
	      used[i+1] = true;
	      sg += signal(i+1);
	    }	
	  else
	    {
	      if(fabs(sn(i+1)) > _neighcut)
		{
		  used[i+1] = true;
		  sg += signal(i+1);
		  left = i+1;
		}
	      else
		break;
	    }
	  
	}
    
	// now to the right
	right = imax;
	for (i=imax+2;i<256;i+=2)
	  {
	    if ( signal(i)*polarity<0.)
	      break;
	    if ( fabs(sn(i)) > _neighcut )
	      {
		used[i] = true;
		sg += signal(i);
		right = i;

		used[i-1] = true;
		sg += signal(i-1);
		
	      }
	    else
	      {
		if(fabs(sn(i-1)) > _neighcut)
		  {
		    used[i-1] = true;
		    sg += signal(i-1);
		    right = i-1;		 
		  }
		else
		  break;	  
	      }
	  }
	add_hit(Hit(imax, left, right, sg));
    }
}
*/

/////mal unverändert
void AsciiRoot::find_clusters(double polarity, int ichip)
{
    int i, imax=-1, left, right;
    double mxsig=-1.e20, sg, sg_cal, val;
    bool used[256];
    int chan0=0;
    int chan1=256;
    
    if (ichip>=0 && ichip<2)
    {
        chan0 = ichip*128;
        chan1 = (ichip+1)*128;
    }
    
    
    for (i=0;i<256;i++)
    {
        used[i]= _mask[i] ? true : false;
    }
    clear();

        
    while (true)
    {
    /////////////
    ///////// Find the highest 
    //////////
        imax = -1;
        for (i=chan0; i<chan1; i++)
        {
	  if (used[i] || signal(i)*polarity<0.)
                continue;
            
            if (fabs(sn(i))>_seedcut)
            {
                val = fabs(signal(i));
                if (mxsig<val)
                {
                    mxsig = val;
                    imax = i;
                }
            }
        }

        if (imax<0)
            break;

        sg = signal(imax);
        sg_cal = signal_cal(imax);
        used[imax]=true;
        // Now look at neighbours
        // first to the left
        left = imax;
        for (i=imax-1;i>=0;i=i-1)
        {
	  if ( signal(i)*polarity<0.)
                break;
            
            if ( fabs(sn(i)) > _neighcut )
            {
                used[i] = true;
                sg += signal(i);
                sg_cal += signal_cal(i);
                left = i;
            }
            else
                break;
        }
        // now to the right
        right = imax;
        for (i=imax+1;i<256;i++)
        {
	  if ( signal(i)*polarity<0.)
                break;
            if ( fabs(sn(i))>_neighcut )
            {
                used[i] = true;
                sg += signal(i);
                sg_cal += signal_cal(i);
                right = i;
            }
            else
                break;
        }
        add_hit(Hit(imax, left, right, sg, sg_cal));
    }
}



void AsciiRoot::save_pedestals(const char *fnam)
{
    std::ofstream ofile(fnam);
    if (!ofile)
    {
        std::cout << "Could not open " << fnam << " to save pedestals." << std::endl;
        return;
    }
    int i;
    for (i=0; i<256; i++)
    {
      ofile << _ped[i] << "\t" << _noise[i] << "\t" << _mask[i] << "\n";
    }
    ofile.close();
}

void AsciiRoot::load_pedestals(const char *fnam)
{
    std::ifstream ifile(fnam);
    if (!ifile)
    {
        std::cout << "Could not open " << fnam << " to load pedestals." << std::endl;
        return;
    }
    int i;
    for (i=0; i<256; i++)
    {
      ifile >> _ped[i] >> std::ws >> _noise[i] >> std::ws >> _mask[i] >> std::ws ;
        //_mask[i] = (_noise[1]>20. || _noise[i]<=0.);
      //        _mask[i] = (_noise[i]>_maxnoise || _noise[i]<_minnoise || _user_mask[i]);  // modified by aldi
	//_mask[i] = false;
	//std::cout << _ped[i] << " " << _noise[i] << " " << _user_mask[i] << std::endl;
    }
//     if(_maskrange1>0 && _maskrange2>0){
//       for (i=_maskrange1;i<=_maskrange2;i++)
// 	_mask[i]=true;
//     }
    ifile.close();
}


void AsciiRoot::set_mask_range(const char *cmask)
{
  const char *p;
  int start,stop;

  start=atoi(cmask);
  p = strchr(cmask,'-');
  stop = atoi(p+1);  
  for(int i=start-1; i<stop;i++)
    _user_mask[i]=true;
  p = strchr(p+1,':');
  std::cout<< "Mask range from " << start << " to " << stop << std::endl;

  while(p!=NULL){

    start=atoi(p+1);
    p = strchr(p+1,'-');
    stop = atoi(p+1);  
    for(int i=start-1; i<stop;i++)
      _user_mask[i]=true;
    p = strchr(p+1,':');
    
    std::cout<< "Mask range from " << start << " to " << stop << std::endl;
  }


}

void AsciiRoot::save_gain(const char *fnam)
{
    std::ofstream ofile(fnam);
    if (!ofile)
    {
        std::cout << "Could not open " << fnam << " to save pedestals." << std::endl;
        return;
    }
    int i;
    for (i=0; i<256; i++)
    {
      ofile << i << "\t" << _gain[i] << "\n";
    }
    ofile.close();
}

void AsciiRoot::load_gain(const char *fnam)
{
    std::ifstream ifile(fnam);
    if (!ifile)
    {
        std::cout << "Could not open " << fnam << " to load the gain." << std::endl;
        return;
    }
    int i;
    int ichan;
    double val, xn, xm;
    xn=xm=0.;
    for (i=0; i<256; i++)
    {
        ifile >> ichan >> std::ws;
        if (ifile.eof())
            break;
        ifile >> val;
        if (ifile.eof())
            break;

	if(val>0 && !_mask[i]){
	  xn++;
	  xm += val;
	  //std::cout << " adding " << val << std::endl;
	}
        _gain[ichan] = val;
        
        ifile >> std::ws;
        if (ifile.eof())
            break;
    }
    if (xn>0)
    {
        _average_gain = xm/xn;
	std::cout << "Average gain is " << _average_gain << std::endl;
    }
    ifile.close();
}

void AsciiRoot::process_event(bool do_cmmd)
{
    int i;
    for (i=0; i<256; i++)
    {

      _signal[i] = _data.data[i]-_ped[i];
      _sn[i] = _noise[i]>1. && !_mask[i] ? _signal[i]/_noise[i] : 0.;
    }
    if (do_cmmd)
    {
        int ichip=-1;
        common_mode_group();    

        for (i=0; i<256; i++)
        {
            if (!(i%128))
                ichip ++;
                
            _signal[i] = _data.data[i]-_ped[i] - _cmmd[ichip];
            _sn[i] = (_noise[i] >1. && !_mask[i] ? _signal[i]/_noise[i] : 0.);
        }
    }
}
void AsciiRoot::common_mode()
{
    int ip, i, j;

    
    for (j=0; j<2; j++)
    {
        int offs = 128*j;
        int sstop = 128*(j+1);
        double mean, sm, xn, xx, xs, xm, tmp;
        bool use_it;
        mean = sm = 0.;
        //std::cout << "Chip " << j << std::endl;    
        for (ip=0;ip<3;ip++)
        {
            xn = xs = xm = 0.;
            for (i=offs; i<sstop; i++)
            {
                if (_mask[i])
                    continue;
                
                use_it = true;
                xx = data(i) - _ped[i];
                if (ip)
                {
                    tmp = fabs((xx-mean)/sm);
                    use_it = (tmp<2.5);
                }
                if (use_it)
                {
                    xn++;
                    xm += xx;
                    xs += xx * xx;
                }
            }
            if (xn>0.)
            {
                mean = xm / xn;
                sm = sqrt( xs/xn - mean*mean);
            }
          //  std::cout << "...iter " << ip << ": xm " << mean << " xs: " << sm << std::endl;
        }
        _cmmd[j] = mean;
        _cnoise[j] = sm;
        
    }
}

void AsciiRoot::common_mode_group()//2012-06-01 initialize dat-array?
{
    int j;
    double dat[256];

    for (j=0; j<CMMD_BLOCKS; j++)
      {
      int offs = (256/CMMD_BLOCKS)*j;
      int sstop = (256/CMMD_BLOCKS)*(j+1);

      int counter = 0;
      for(int i=offs;i<sstop;i++){
	if(!_mask[i]){
	  dat[counter]=_data.data[i]-_ped[i];
	  counter++;
	}
      }

      _cmmd[j] = TMath::Median(counter,dat);
      
      }
    
}

void AsciiRoot::spy_time(int nevt)
{
  TCanvas *cnvs = (TCanvas *)gROOT->FindObject("cnvs");
  if (cnvs)
    {
      cnvs->Clear();
    }
  else
    cnvs = new TCanvas("cnvs","cnvs", 800, 400);
  cnvs->Divide(1,3);

  
  TH1 *htime = create_h1("htime","Time",500, -250., 250.);
  htime->SetXTitle("Time");
  htime->SetYTitle("#");
 
  TH1 *htemp = create_h1("htemp","Temp",100, -50., 50.);
  htemp->SetXTitle("Temp");
  htemp->SetYTitle("#");

  TH1 *hsig = create_h1("hsig","Signal",2048, -1024., 1024.);
  htemp->SetXTitle("Sig");
  htemp->SetYTitle("#");
  
  int ievt,jevt;
  for (ievt=jevt=0; read_event()==0 && _A_do_run && ievt<nevt;jevt++)
    {
      process_event(true);
      std::cout << "Event: " << ievt << "  Clock: " << clock()<< "  Time: " << time() << " ns" << "  Temp: " << temp() << " C" << std::endl;
      htime->Fill(time());
      htemp->Fill(temp());
      hsig->Fill(_signal[0]);
      ievt++;
    }
  cnvs->cd(1);
  htime->Draw();
  cnvs->cd(2);
  htemp->Draw();
  cnvs->cd(3);
  hsig->Draw();

}



void AsciiRoot::spy_data(bool with_signal, int nevt, double polarity, int first_strip, int last_strip)
{
    TVirtualPad *pad;
    if (!ifile)
        return;
    
    sighandler_t old_handler = ::signal(SIGINT, _A_got_intr);
    _A_do_run = true;

    TCanvas *cnvs = (TCanvas *)gROOT->FindObject("cnvs");
    if (cnvs)
    {
        cnvs->Clear();
    }
    else
       cnvs = new TCanvas("cnvs","cnvs", 700, 800);
    
    cnvs->Divide(2,3);
    
    
    TH1 *hsignal = create_h1("hsignal","signal (eADC)", 256, -0.5, 255.5);
    hsignal->SetXTitle("Channel");
    hsignal->SetYTitle("ADC");
    hsignal->SetMinimum(-150);
    hsignal->SetMaximum(150);
    if(polarity>0){
      hsignal->SetMinimum(-5);
      hsignal->SetMaximum(500);
    }
    if(polarity<0){
      hsignal->SetMinimum(-500);
      hsignal->SetMaximum(5);
    }
    TH1 *helec = create_h1("helec","signal (elec)", 256, -0.5, 255.5);
    helec->SetXTitle("Channel");
    helec->SetYTitle("electrons");
    helec->SetMinimum(-150/gain());
    helec->SetMaximum(150/gain());
    if(polarity>0)
      helec->SetMinimum(-5);
    if(polarity<0)
      helec->SetMaximum(5);

    
    TH1 *hraw = create_h1("hraw","Raw Data (around 512.)",256, 0., 256.);
    hraw->SetXTitle("Channel");
    hraw->SetYTitle("ADC");
    hraw->SetMinimum(-150);
    hraw->SetMaximum(+150);
    
    TH1 *hrawc = create_h1("hrawc","Raw Data (no commd)",256, 0., 256.);
    hrawc->SetXTitle("Channel");
    hrawc->SetYTitle("ADC");
    hrawc->SetMinimum(-150);
    hrawc->SetMaximum(+150);
    

    TH1 *hcmmd[2];
    hcmmd[0] = create_h1("hcmmd0","Common mode (Chip 0)",50,-100.,100.);
    hcmmd[0]->SetXTitle("Common mode");
    hcmmd[1] = create_h1("hcmmd1","Common mode (Chip 1)",50,-100.,100.);
    hcmmd[1]->SetXTitle("Common mode");
    
    int ievt,jevt;
    for (ievt=jevt=0; read_event()==0 && _A_do_run && ievt<nevt;jevt++)
    {
        process_event(true);
        find_clusters(polarity);
        if ( with_signal && empty())
            continue;
        
        int i,ichip=-1;
        for (i=0; i<256; i++)
        {
            if (!(i%128))
                ichip++;
            
            hsignal->SetBinContent(i+1, _signal[i]);
            helec->SetBinContent(i+1, signal(i));
            hraw->SetBinContent(i+1,data(i)-512.);
            hrawc->SetBinContent(i+1, data(i)-_ped[i]);
            hcmmd[ichip]->Fill(_signal[i]+get_cmmd(ichip));
        }
        pad = cnvs->cd(1);
        pad->SetGrid(1,1);
        hsignal->Draw();
	hsignal->GetXaxis()->SetRangeUser(first_strip,last_strip);
        pad = cnvs->cd(2);
        pad->SetGrid(1,1);
        helec->Draw();
        
        pad = cnvs->cd(3);
        pad->SetGrid(1,1);
        hraw->Draw();
        
        pad = cnvs->cd(4);
        pad->SetGrid(1,1);
        hrawc->Draw();

        pad = cnvs->cd(5);
        pad->SetGrid(1,1);
        hcmmd[0]->Draw();

        pad = cnvs->cd(6);
        pad->SetGrid(1,1);
        hcmmd[1]->Draw();
            
        std::cout << std::setiosflags(std::ios::fixed);
        std::cout << "*** Event " << jevt << " *****" << std::endl;
        std::cout << "Common Mode:" << std::endl
                  << "   Chip 0 " << std::setw(6) << std::setprecision(1) << get_cmmd(0) << " noise: " << get_cnoise(0)
                  << std::endl
                  << "   Chip 1 " << std::setw(6) << std::setprecision(1) << get_cmmd(1) << " noise: " << get_cnoise(1)
                  << std::endl;   
                  
        std::cout << "Time: " << time() << " ns" << std::endl;
        std::cout << "Temp: " << temp() << " C" << std::endl;
        std::cout << "Clusters: " << std::endl;

        AsciiRoot::HitList::iterator ip;
        for (ip=begin(); ip!=end(); ++ip)
        {
            std::cout << "   chan: " << ip->center() 
                      << " sig: "
                      << std::setw(6) << std::setprecision(1) << ip->signal()
                      << " left: " << ip->left() << " right: " << ip->right()
                      << std::endl;
            std::cout << '\t' << "channels: " << std::endl;
            int j;
            for (j=ip->left();j<=ip->right();j++)
                std::cout << "\t   " << j << " sn: " << _sn[j] << " signal: " << _signal[j] << " noise: " << _noise[j] << '\n';
            std::cout << std::endl;
        }
        
        cnvs->Update();
        ievt++;
    }
    std::cout << std::endl;
    _A_do_run= true;
    ::signal(SIGINT, old_handler);
}

TH1*  AsciiRoot::show_noise_plots(bool with_signal, double polarity, bool issignalrun)
{
	//TCanvas* noisecanvas = new TCanvas("noisecanvas","noisecanvas",1200,600);
	//noisecanvas->Divide(2,1);
	TH1D* striphisto = new TH1D("noisehistogram","Noise Histogram",401,-200,200);
	TGraph* g = new TGraph();

	int ievt,jevt;
        int stripcounter = 0;

 	int nevt=0;

	for (ievt=jevt=0; read_event()==0 && _A_do_run /*&& ievt<nevt*/;jevt++)
    	{
		nevt++;
		process_event();
		find_clusters(polarity);
		if ( with_signal && empty())
		continue;
//cout << "event: " << jevt << endl;		
		int i,ichip=-1;
		for (i=0; i<=255; i++)
		{
//cout << i<< " " << _signal[i] << endl; 
			//striphisto->Fill(data(i)-512.);
		  //			if (_mask[i]==0 && time()>43 && time()<70 && issignalrun || _mask[i]==0 && !issignalrun)
			if (_mask[i]==0 && issignalrun || _mask[i]==0 && !issignalrun)
			{
				striphisto->Fill(_signal[i]*polarity);
				if (jevt==0)
					stripcounter++;
			}
			if (jevt==0)
			{
				
				
				g->SetPoint(g->GetN(),i,data(i)-512.);
			}
			//striphisto->Fill(data(i)-_ped[i]);

			//hsignal->SetBinContent(i+1, _signal[i]);
	// 		helec->SetBinContent(i+1, signal(i));
	// 		hraw->SetBinContent(i+1,data(i)-512.);
	// 		hrawc->SetBinContent(i+1, data(i)-_ped[i]);
	// 		hcmmd[ichip]->Fill(_signal[i]+get_cmmd(ichip));
		}
        }
	
	//noisecanvas->cd(1);
	if (striphisto->GetEntries()>0)
	{
		striphisto->Fit("gaus");
		//striphisto->Draw();
		//noisecanvas->GetPad(1)->SetLogy();
		//noisecanvas->cd(2);
		//TH1D* striphistoclone;
		//striphistoclone("striphisto");
		
		//striphisto->Draw();
		//striphisto->Clone()->Draw();
			
		//Find bin with 5 sigma 
		TF1* gauss=striphisto->GetFunction("gaus");
		double mean=gauss->GetParameter(1);
		double sigma=gauss->GetParameter(2);
		int start_bin=striphisto->FindBin(mean+5*sigma);
		//std::cout <<"Binnummer mit 5 Sigma :"<< start_bin  << " Mean: " << striphisto->FindBin(mean)<<  " SIGMA " << sigma << " mean+5sigma " <<  mean+5*sigma <<std::endl;
	
		//Bincontent > 5 sigma finden
		double summe_bincontent=0;
		
		for(int bin=start_bin; bin<striphisto->GetNbinsX();bin++)
		{	
			summe_bincontent=summe_bincontent+striphisto->GetBinContent(bin);
		}
		
		//std::cout<<"Summe Bincontent < 5Sigma: "<<summe_bincontent<<std::endl; 
	//cout << "stripcounter " << stripcounter<<endl;
		noisevalue = (double)summe_bincontent / (double)nevt / stripcounter;
	}
	return striphisto;
	//c->cd(3);
	//g->Draw("AP*");
}
