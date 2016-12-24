/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir simulation functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cassert>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"


using namespace std;

// TODO: Keep "CirMgr::randimSim()" and "CirMgr::fileSim()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/
/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/************************************************/
/*   Public member functions about Simulation   */
/************************************************/
void
CirMgr::randomSim()
{
}

void
CirMgr::fileSim(ifstream& patternFile)
{
   if(!patternFile.is_open()) {
      return;
   }
   string str;
   size_t _i = miloa[1];
   vector<string> inputs;
   while(patternFile >> str) {
      if(str.size() != _i && str.size() != 0) {
         cerr << "Error: Pattern(" << str <<") length(" << str.size() 
              << ") does not match the number of inputs("
              << _i << ") in a circuit!!" << endl;
         cout << "\r" << "0 patterns simulated." << endl;
         return;
      }  
      inputs.push_back(str);
   }
   unsigned div = 0, mod = inputs.size() % BIT_32; // div: num of round, e.g. 64 is 2 rounds
   if(mod != 0) {
      div = inputs.size() / BIT_32;
      mod = BIT_32 * (div + 1) - inputs.size();
   }
   for(unsigned j = 0; j < mod; j ++) {
      char zero_str [_i];
      memset(zero_str, '0', sizeof(zero_str));
      zero_str[_i] = '\0';
      inputs.push_back(zero_str);
   }
   div = inputs.size() / BIT_32;
   vector<size_t> _32bitvec;
   for(unsigned i = 0; i < div; i++) {
      for(unsigned j = 0; j < _i; j++) {
         size_t pattern = 0;
         for(unsigned k = 0; k < BIT_32; k++) {
            int new_bit = inputs[k + i * BIT_32][j] - '0';
            if(new_bit != 0 && new_bit != 1) {
               cerr << "Error: Pattern(" << inputs[k + i * BIT_32] 
                    << ") contains a non-0/1 character('" 
                    << inputs[k + i * BIT_32][j] << "')." << endl;
               cout << "\r" << "0 patterns simulated." << endl;
               return;
            } 
            pattern = (pattern << 1) + new_bit; // key!
         }
         _32bitvec.push_back(pattern);
      }
   }
   // start sim
   buildDfsList();
   unsigned a = 0;
   fecGrpsInit();
   for( ; a < div; a++) {
      simulate(a, _32bitvec);
      fecGrpsUpdate();
   }

   // cout << "\rTotal #FEC Group = " << _fecList.numGroups();
   cout << "\r" << div * BIT_32 - mod << " patterns simulated." << endl;

}

/*************************************************/
/*   Private member functions about Simulation   */
/*************************************************/

void
CirMgr::simulate(unsigned& round, vector<size_t>& _32bitvec)
{
   unsigned ins_index = 0;
   unsigned _i = miloa[1];
   for(unsigned i = 0; i < _dfsList.size(); i++) {
      CirGate* g = _dfsList[i];
      if(g->getType() == PI_GATE || g->getType() == CONST_GATE) {
      
         CirPiGate* pi = (CirPiGate*) g;
         ins_index = pi->getPiIndex();
         size_t pattern = _32bitvec[round * _i + ins_index]; //
         g->setLastValue(pattern);
      } else if(g->getType() == AIG_GATE) {
         CirGate* lhs0 = g->getInput(0);
         CirGate* lhs1 = g->getInput(1);
         size_t v0 = lhs0->getLastValue(), v1 = lhs1->getLastValue();
         if(g->isInv(0)) v0 = ~v0;
         if(g->isInv(1)) v1 = ~v1;
         g->setLastValue(v0 & v1);
      } else if (g->getType() == UNDEF_GATE) {
         g->setLastValue(0);
      } else { // PO_GATE
         CirGate* lhs = g->getInput(0);
         size_t v0 = lhs->getLastValue();
         if(g->isInv(0)) v0 = ~v0;
         g->setLastValue(v0);
      }
   }
}
void
CirMgr::fecGrpsInit()
{
   FecGrp *fecGrp = new FecGrp();
   for(unsigned i = 0; i < _dfsList.size(); i++) {
      CirGate* g = _dfsList[i];
      if(g->getType() == AIG_GATE) fecGrp->addGate(g);
   }
   fecGrps.push_back(fecGrp);
}

void
CirMgr::fecGrpsUpdate()
{
   unsigned _m = miloa[0], _o = miloa[3];
   FecMap newFecMap( getHashSize(_m + _o + 1) );

   // for(unsigned i = 0; i < fecGrps.size(); i ++) {
   FecGrp* fecGrp = fecGrps[0];
   for(unsigned j = 0; j < fecGrp->getGatesSize(); j++) {
      CirGate *g = fecGrp->getGate(j);
      size_t val = g->getLastValue();
      FecHashKey key(val);
      FecGrp* tmpGrp = NULL;
      if(newFecMap.query(key, tmpGrp)) {
         tmpGrp->addGate(g);
      } else {
         // tmpGrp is NULL
         createNewGroup(newFecMap, g);
      }
   }
   // }
}
void
CirMgr::createNewGroup(FecMap& newFecMap, CirGate* g)
{
   size_t val = g->getLastValue();
   FecGrp *fecGrp = new FecGrp(val);
   fecGrp->addGate(g);
   fecGrps.push_back(fecGrp);
   FecHashKey key(val);
   newFecMap.insert(key, fecGrp);
}