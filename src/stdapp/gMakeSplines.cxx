//____________________________________________________________________________
/*!

\program gmkspl

\brief   GENIE utility program building XML cross section splines that can
         be loaded into GENIE to speed-up event generation.
         The list of neutrino PDG codes is passed from the command line.
         The list of nuclear target PDG codes is either passed from the 
         command line or extracted from the input ROOT/GEANT geometry.

         Syntax :
           gmkspl -p nupdg <-t tgtpdg, -f geomfile> [-o output_xml_file]
                  [-n nknots] [-e max_energy]
         Note :
           [] marks optional arguments.
           <> marks a list of arguments out of which only one can be 
              selected at any given time.

         Options :
           -p  a comma separated list of nu PDG code
           -t  a comma separated list of tgt PDG codes (format: 1aaazzz000)
           -f  a ROOT file containing a ROOT/GEANT geometry description
           -o  output XML filename [ default: xsec_splines.xml ]
           -n  number of knots per spline [default: 15 knots per decade of
               energy range with a minimum of 30 knots totally]
           -e  max energy in spline [default: the max energy in the validity
               range of the spline generating thread]

         Examples:

         1) gmkspl -p 14,-14 -t 1056026000

            will build cross section splines for muon neutrinos (pdg = 14)
            and muon anti-neutrinos (pgc = -14) on Iron (A=56,Z=26) and will
            save them in an XML file with the default name.

         2) gmkspl -p 14,-14 -f ~/data/mygeometry.root -o spl.xml

            will build cross section splines for muon neutrinos (pdg = 14)
            and muon anti-neutrinos (pgc = -14) on all the materials found
            in the input ROOT geometry (found at mygeometry.root) and will
            save them in an XML file named spl.xml

         Other control options:

         You can further control the program behaviour by setting the GEVGL
         and GMSGCONF environmental variables.

           - Set the GEVGL environmental variable to contol the list of event
             generator objects that will get loaded into the event generation
             driver (see $GENIE/src/stdapp/gEvGen.cxx). This program will only
             build splines for the processes that can be simulated by the event
             generators you plan to load.

           - You can set the GMSGCONF env.variable to point to a messenger
             XML configuration file (following the syntax of the default one
             that can be found in $GENIE/config/messenger.xml) and modify the
             verbosity of GENIE output. Both the default and your messenger
             configuration will be read but yours will take precedence in
             case of clashing priority for the same stream.

\author  Costas Andreopoulos <C.V.Andreopoulos@rl.ac.uk>
         CCLRC, Rutherford Appleton Laboratory

\created September 27, 2005

\cpright Copyright (c) 2003-2006, GENIE Neutrino MC Generator Collaboration
         All rights reserved.
         For the licensing terms see $GENIE/USER_LICENSE.
*/
//____________________________________________________________________________

#include <cassert>
#include <string>
#include <vector>

#include <TSystem.h>

#include "EVGDrivers/GEVGDriver.h"
#include "Geo/ROOTGeomAnalyzer.h"
#include "Interaction/Interaction.h"
#include "Messenger/Messenger.h"
#include "PDG/PDGCodeList.h"
#include "Utils/StringUtils.h"
#include "Utils/XSecSplineList.h"
#include "Utils/CmdLineArgParserUtils.h"
#include "Utils/CmdLineArgParserException.h"

using std::string;
using std::vector;

using namespace genie;
using namespace genie::geometry;

//Prototypes:
void          GetCommandLineArgs (int argc, char ** argv);
void          PrintSyntax        (void);
PDGCodeList * GetNeutrinoCodes   (void);
PDGCodeList * GetTargetCodes     (void);

//Defaults for optional options:
string kDefOptXMLFilename = "xsec_splines.xml";

//User-specified options:
string gOptNuPdgCodeList  = "";
string gOptTgtPdgCodeList = "";
string gOptGeomFilename   = "";
string gOptXMLFilename    = "";
int    gOptNKnots         = -1;
double gOptMaxE           = -1.;

//____________________________________________________________________________
int main(int argc, char ** argv)
{
  //-- parse command line arguments
  GetCommandLineArgs(argc,argv);

  //-- print the options you got from command line arguments
  LOG("gmkspl", pINFO) << "Neutrino PDG codes  = " << gOptNuPdgCodeList;
  LOG("gmkspl", pINFO) << "Target PDG codes    = " << gOptTgtPdgCodeList;
  LOG("gmkspl", pINFO) << "Input ROOT geometry = " << gOptGeomFilename;
  LOG("gmkspl", pINFO) << "Output XML file     = " << gOptXMLFilename;

  PDGCodeList * neutrinos = GetNeutrinoCodes();
  PDGCodeList * targets   = GetTargetCodes();

  if(!neutrinos || neutrinos->size() == 0 ) {
     LOG("gmkspl", pFATAL) << "Empty neutrino PDG code list";
     PrintSyntax();
     exit(2);
  }
  if(!targets || targets->size() == 0 ) {
     LOG("gmkspl", pFATAL) << "Empty target PDG code list";
     PrintSyntax();
     exit(3);
  }

  LOG("gmkspl", pINFO) << "Neutrinos: " << *neutrinos;
  LOG("gmkspl", pINFO) << "Targets: "   << *targets;

  //-- loop over all possible input init states and ask the GEVGDriver
  //   to build splines for all the interactions that its loaded list
  //   of event generators can generate.

  PDGCodeList::const_iterator nuiter;
  PDGCodeList::const_iterator tgtiter;

  for(nuiter = neutrinos->begin(); nuiter != neutrinos->end(); ++nuiter) {
    for(tgtiter = targets->begin(); tgtiter != targets->end(); ++tgtiter) {

      int nupdgc  = *nuiter;
      int tgtpdgc = *tgtiter;

      InitialState init_state(tgtpdgc, nupdgc);

      GEVGDriver driver;

      driver.Configure(init_state);
      driver.CreateSplines(gOptNKnots, gOptMaxE);
    }
  }

  //-- get the populated cross section spline list and save it at the
  //   requested XML file

  XSecSplineList * spline_list = XSecSplineList::Instance();

  spline_list->SaveAsXml(gOptXMLFilename);

  delete neutrinos;
  delete targets;

  return 0;
}
//____________________________________________________________________________
void GetCommandLineArgs(int argc, char ** argv)
{
  LOG("gmkspl", pINFO) << "Parsing commad line arguments";

  //-- Optional arguments

  //output XML file name:
  try {
    LOG("gmkspl", pINFO) << "Reading output filename";
    gOptXMLFilename = genie::utils::clap::CmdLineArgAsString(argc,argv,'o');
  } catch(exceptions::CmdLineArgParserException e) {
    if(!e.ArgumentFound()) {
      LOG("gmkspl", pINFO) << "Unspecified filename - Using default";
      gOptXMLFilename = kDefOptXMLFilename;
    }
  }

  //number of knots:
  try {
    LOG("gmkspl", pINFO) << "Reading number of knots/spline";
    gOptNKnots = genie::utils::clap::CmdLineArgAsInt(argc,argv,'n');
  } catch(exceptions::CmdLineArgParserException e) {
    if(!e.ArgumentFound()) {
      LOG("gmkspl", pINFO)
            << "Unspecified number of knots - Using default";
      gOptNKnots = -1;
    }
  }

  //max spline energy (if < max of validity range)
  try {
    LOG("gmkspl", pINFO) << "Reading maximum spline energy";
    gOptMaxE = genie::utils::clap::CmdLineArgAsDouble(argc,argv,'e');

  } catch(exceptions::CmdLineArgParserException e) {
    if(!e.ArgumentFound()) {
      LOG("gmkspl", pINFO) 
             << "Unspecified maximum spline energy - Using default";
      gOptMaxE = -1;
    }
  }


  //-- Required arguments

  //comma-separated neutrino PDG code list:

  try {
    LOG("gmkspl", pINFO) << "Reading neutrino PDG codes from command line";
    gOptNuPdgCodeList = genie::utils::clap::CmdLineArgAsString(argc,argv,'p');
  } catch(exceptions::CmdLineArgParserException e) {
    if(!e.ArgumentFound()) {
      LOG("gmkspl", pFATAL) << "Unspecified neutrino PDG code list - Exiting";
      PrintSyntax();
      exit(1);
    }
  }

  //comma-separated target PDG code list or input geometry file:

  bool tgt_cmd = true;
  try {
    LOG("gmkspl", pINFO) << "Reading target nuclei PDG codes from command line";
    gOptTgtPdgCodeList = genie::utils::clap::CmdLineArgAsString(argc,argv,'t');
  } catch(exceptions::CmdLineArgParserException e) {
    if(!e.ArgumentFound()) {
      LOG("gmkspl", pINFO) << "No code list specified from the command line";
      tgt_cmd = false;
    }
  }

  bool tgt_geom = true;
  try {
    LOG("gmkspl", pINFO) << "Reading ROOT/GEANT geometry filename";
    gOptGeomFilename = genie::utils::clap::CmdLineArgAsString(argc,argv,'f');
  } catch(exceptions::CmdLineArgParserException e) {
    if(!e.ArgumentFound()) {
      LOG("gmkspl", pINFO) << "No geometry file was specified";
      tgt_cmd = false;
    }
  }

  bool both =  tgt_geom &&  tgt_cmd;
  bool none = !tgt_geom && !tgt_cmd;

  if(none) {
    LOG("gmkspl", pFATAL) 
          << "No geom file or cmd line target list was specified - Exiting";
    PrintSyntax();
    exit(1);
  }

  if(both) {
    LOG("gmkspl", pFATAL) 
       << "You specified both a geom file and a cmd line target list "
         << "- Exiting confused";
    PrintSyntax();
    exit(1);
  }
}
//____________________________________________________________________________
void PrintSyntax(void)
{
  LOG("gmkspl", pNOTICE)
    << "\n\n" << "Syntax:" << "\n"
    << "   gmkspl -p nupdg <-t tgtpdg, -f geomfile> [-o output_xml]"
    << " [-n nknots] [-e max_energy]";
}
//____________________________________________________________________________
PDGCodeList * GetNeutrinoCodes(void)
{
  // split the comma separated list
  vector<string> nuvec = utils::str::Split(gOptNuPdgCodeList,  ",");

  // fill in the PDG code list
  PDGCodeList * list = new PDGCodeList;
  vector<string>::const_iterator iter;
  for(iter = nuvec.begin(); iter != nuvec.end(); ++iter) {
    list->push_back( atoi(iter->c_str()) );
  }
  return list;
}
//____________________________________________________________________________
PDGCodeList * GetTargetCodes(void)
{
  bool from_geom_file = ( gOptGeomFilename.size()   > 0 );
  bool from_cmd_line  = ( gOptTgtPdgCodeList.size() > 0 );

  if (from_cmd_line) {
     // split the comma separated list
     vector<string> tgtvec = utils::str::Split(gOptTgtPdgCodeList, ",");

     // fill in the PDG code list
     PDGCodeList * list = new PDGCodeList;
     vector<string>::const_iterator iter;
     for(iter = tgtvec.begin(); iter != tgtvec.end(); ++iter) {
        list->push_back( atoi(iter->c_str()) );
     }
     return list;
  }

  if (from_geom_file) {
     // create/configure a geometry driver
     LOG("gmkspl", pINFO) << "Creating/configuring a ROOT geom. driver";
     ROOTGeomAnalyzer * geom = new ROOTGeomAnalyzer(gOptGeomFilename);

     PDGCodeList * list = new PDGCodeList(geom->ListOfTargetNuclei());

     delete geom;
     return list;
  }
  return 0;
}
//____________________________________________________________________________

