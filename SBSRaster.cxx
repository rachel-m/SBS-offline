#include "SBSRaster.h"
using namespace std;
static const UInt_t NPOS = 3;
static const UInt_t NBPM = 2;
//_____________________________________________________________________________
SBSRaster::SBSRaster( const char* name, const char* description,
				  THaApparatus* apparatus )
  : THaBeamDet(name,description,apparatus), fRawPos(NBPM), fRawSlope(NBPM),
    fRasterFreq(NBPM), fSlopePedestal(NBPM), fRasterPedestal(NBPM)
{
  // Constructor
  fRaw2Pos[0].ResizeTo(NPOS,NBPM);
  fRaw2Pos[1].ResizeTo(NPOS,NBPM);
  fRaw2Pos[2].ResizeTo(NPOS,NBPM);

}
//_____________________________________________________________________________
Int_t SBSRaster::ReadDatabase( const TDatime& date )
{
  // ReadDatabase:  if detectors cant be added to detmap
  //                or entry for bpms is missing           -> kInitError
  //                otherwise                              -> kOk

  const char* const here = "ReadDatabase";
  vector<Int_t> detmap;
  Double_t zpos[NPOS];
  Double_t freq[NBPM], rped[NBPM], sped[NBPM];
  Double_t raw2posA[NBPM*NPOS], raw2posB[NBPM*NPOS], raw2posT[NBPM*NPOS];

  std::cout << GetDBFileName() << std::endl;

  FILE* file = OpenFile( date );
  if( !file )
    return kInitError;

  // fOrigin.SetXYZ(0,0,0);
  Int_t err = kOK;// = ReadGeometry( file, date );

  if( !err ) {
    memset( zpos, 0, sizeof(zpos) );
    // Read configuration parameters
    DBRequest config_request[] = {
      { "detmap",   &detmap,  kIntV },
      { "zpos",     zpos,     kDouble, NPOS, 1 },
      { 0 }
    };
    err = LoadDB( file, date, config_request, fPrefix );
  }

  UInt_t flags = THaDetMap::kFillLogicalChannel | THaDetMap::kFillModel;
  if( !err && FillDetMap(detmap, flags, here) <= 0 ) {
    err = kInitError;  // Error already printed by FillDetMap
  }

  if( !err ) {
    fPosOff[0].SetZ( zpos[0] );
    fPosOff[1].SetZ( zpos[1] );
    fPosOff[2].SetZ( zpos[2] );

    memset( rped, 0, sizeof(rped) );
    memset( sped, 0, sizeof(sped) );

    DBRequest calib_request[] = {
      { "freqs",      freq,     kDouble, NBPM },
      { "rast_peds",  rped,     kDouble, NBPM, 1 },
      { "slope_peds", sped,     kDouble, NBPM, 1 },
      { "raw2posA",   raw2posA, kDouble, NBPM*NPOS },
      { "raw2posB",   raw2posB, kDouble, NBPM*NPOS },
      { "raw2posT",   raw2posT, kDouble, NBPM*NPOS },
      { 0 }
    };
    err = LoadDB( file, date, calib_request, fPrefix );
  }

  fRasterFreq.SetElements( freq );
  fRasterPedestal.SetElements( rped );
  fSlopePedestal.SetElements( sped );

  fPosOff[0].SetX(   raw2posA[0]);
  fPosOff[0].SetY(   raw2posA[1]);
  fRaw2Pos[0](0,0) = raw2posA[2];
  fRaw2Pos[0](1,1) = raw2posA[3];
  fRaw2Pos[0](0,1) = raw2posA[4];
  fRaw2Pos[0](1,0) = raw2posA[5];

  fPosOff[1].SetX(   raw2posB[0]);
  fPosOff[1].SetY(   raw2posB[1]);
  fRaw2Pos[1](0,0) = raw2posB[2];
  fRaw2Pos[1](1,1) = raw2posB[3];
  fRaw2Pos[1](0,1) = raw2posB[4];
  fRaw2Pos[1](1,0) = raw2posB[5];

  fPosOff[2].SetX(   raw2posT[0]);
  fPosOff[2].SetY(   raw2posT[1]);
  fRaw2Pos[2](0,0) = raw2posT[2];
  fRaw2Pos[2](1,1) = raw2posT[3];
  fRaw2Pos[2](0,1) = raw2posT[4];
  fRaw2Pos[2](1,0) = raw2posT[5];

  // char path[1024];
  // char result[1024];
  // int fd = fileno(file);
  // sprintf(path,"/proc/self/df/%d",fd);
  // memset(result,0,sizeof(result));
  // readlink(path,result,sizeof(result)-1);
  // printf(Form("File name: %s\n",result));
  // printf(Form("%d",fileno(file))); // /proc/self/fd/number...
  printf(Form("OFFSETs and Scaler Factors of Raster for file %d: %f, %f, %f, %f\n",date.GetDate(),raw2posT[0],raw2posT[1],rped[0],rped[1]));
  fclose(file);
  if( err )
    return err;
  return kOK;
}
//______________________________________________________________________________
Int_t SBSRaster::DefineVariables( EMode mode )
{
  // Initialize global variables and lookup table for decoder

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  // Register variables in global list
  
  RVarDef vars[] = {
    { "rawcur.x", "current in horizontal raster", "GetRawPosX()" },
    { "rawcur.y", "current in vertical raster", "GetRawPosY()"},
    { "rawslope.x", "derivative of current in horizontal raster", "GetRawSlopeX()" },
    { "rawslope.y", "derivative of current in vertical raster", "GetRawSlopeY()"},
    { "bpma.x", "reconstructed x-position at 1st bpm", "GetPosBPMAX()"},
    { "bpma.y", "reconstructed y-position at 1st bpm", "GetPosBPMAY()"},
    { "bpma.z", "reconstructed z-position at 1st bpm", "GetPosBPMAZ()"},
    { "bpmb.x", "reconstructed x-position at 2nd bpm", "GetPosBPMBX()"},
    { "bpmb.y", "reconstructed y-position at 2nd bpm", "GetPosBPMBY()"},
    { "bpmb.z", "reconstructed z-position at 2nd bpm", "GetPosBPMBZ()"},
    { "target.x", "reconstructed x-position at nom. interaction point", "GetPosTarX()"},
    { "target.y", "reconstructed y-position at nom. interaction point", "GetPosTarY()"},
    { "target.z", "reconstructed z-position at nom. interaction point", "GetPosTarZ()"},
    { "target.dir.x", "reconstructed x-component of beam direction", "fDirection.fX"},
    { "target.dir.y", "reconstructed y-component of beam direction", "fDirection.fY"},
    { "target.dir.z", "reconstructed z-component of beam direction", "fDirection.fZ"},
    { 0 }
  };
    
  return DefineVarsFromList( vars, mode );

}
//______________________________________________________________________________
SBSRaster::~SBSRaster()
{
  // Destructor. Remove variables from global list.

  if( fIsSetup )
    RemoveVariables();

}
//______________________________________________________________________________
void SBSRaster::Clear( Option_t* )
{
  // Reset per-event data.
  fRawPos(0)=-1;
  fRawPos(1)=-1;
  fRawSlope(0)=-1;
  fRawSlope(1)=-1;
  fPosition[0].SetXYZ(0.,0.,-10000.);
  fPosition[1].SetXYZ(0.,0.,-10000.);
  fPosition[2].SetXYZ(0.,0.,-10000.);
  fDirection.SetXYZ(0.,0.,1.);
  fNfired=0;
}
//______________________________________________________________________________
Int_t SBSRaster::Decode( const THaEvData& evdata )
{

  // clears the event structure
  // loops over all modules defined in the detector map
  // copies raw data into local variables
  // pedestal subtraction is not foreseen for the raster

  const char* const here = "Decode()";

  // UInt_t chancnt = 0;

  // to eliminate compilation warnings 
  Int_t detMapSize = fDetMap->GetSize();
  Int_t numCh=0,data=0;
  UInt_t k=0,chan=0,chancnt=0;


  for (Int_t i = 0; i < detMapSize; i++ ){
    THaDetMap::Module* d = fDetMap->GetModule( i );
    numCh = evdata.GetNumChan( d->crate, d->slot );
    for (Int_t j=0; j< numCh; j++) {
      chan = evdata.GetNextChan( d->crate, d->slot, j);
      if ((chan>=d->lo)&&(chan<=d->hi)) {  
	data = 0; 
	k = chancnt + d->first + ((d->reverse) ? d->hi - chan : chan - d->lo) -1;
	if (k<NBPM) {
          data = evdata.GetData( Decoder::kPulseIntegral,d->crate,d->slot,chan,0 );
	  fRawPos(k)= data; 
	  fNfired++;  
	}
	else if (k<2*NBPM) {
          data = evdata.GetData( d->crate,d->slot,chan,0 );
	  fRawSlope(k-NBPM)= data;
	  fNfired++;  
	}
	else {
	  Warning( Here(here), "Illegal detector channel: %d", k );
	}
      }


    }

    chancnt+=d->hi-d->lo+1;
  }
  // condition was set to NBPM to silence the Warnings, since the parameters for 
  // fRawSlope were not used in the recent years and they are also not connected
  char msg[200];
  sprintf(msg,"====> DEBUG: Nfired = %d, NBPM = %d",(int)fNfired,(int)NBPM);
  if (fNfired!=NBPM) {
      Warning( Here(here), "Number of fired Channels out of range. "
	       "Setting beam position to nominal values");
      // THaDetMap::Print() format is: crate, slot, lo, hi, first, model-type, refchan, refindex, resolution, plane, signal
      std::cout << "----------------------------------------------" << std::endl;
      std::cout << "Event " << evdata.GetEvNum() << std::endl;
      std::cout << msg << std::endl;
      std::cout << "SBSRaster::Decode() calling THaDetMap::Print()" << std::endl;
      fDetMap->Print();
      std::cout << "SBSRaster: detMapSize = " << detMapSize << std::endl;
  }
  return 0;
}
//______________________________________________________________________________
Int_t SBSRaster::Process( )
{

  for ( UInt_t i = 0; i<NPOS; i++) {
    // fPosition[i] = fRaw2Pos[i]*fRawPos+fPosOff[i] ;
    // this is how i wish it would look like,
    // but unluckily multiplications between tmatrix and tvector
    // are not defined, as well as adding a tvector and a tvector3
    // so i have to do it by hand instead ):
    TVectorD dum(fRawPos);
    dum*=fRaw2Pos[i];
    fPosition[i].SetXYZ( dum(0)+fPosOff[i](0),
			 dum(1)+fPosOff[i](1),
			 dum(2)+fPosOff[i](2)  );

  }
  
  fDirection = fPosition[1] - fPosition[0];

  return 0 ;
}
//______________________________________________________________________________
ClassImp(SBSRaster)
