/****************************************************************************
  FileName     [ cirGate.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define class CirAigGate member functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdarg.h>
#include <cassert>
#include "cirGate.h"
#include "cirMgr.h"
#include "util.h"

using namespace std;

// TODO: Keep "CirGate::reportGate()", "CirGate::reportFanin()" and
//       "CirGate::reportFanout()" for cir cmds. Feel free to define
//       your own variables and functions.

extern CirMgr *cirMgr;

/**************************************/
/*   class CirGate member functions   */
/**************************************/
unsigned CirGate::index = 0;
unsigned CirGate::globalRef = 0;

string 
CirGate::getTypeStr() const 
{
   if(type == PI_GATE)
      return "PI";
   else if(type == PO_GATE)
      return "PO";
   else if(type == AIG_GATE)
      return "AIG";
   else if(type == CONST_GATE)
      return "CONST";
   else if(type == UNDEF_GATE)
      return "UNDEF";
   return "UNDEF";
}
string unsToStr(unsigned n) {
   stringstream ss;
   ss << n;
   return ss.str();
}
bool orderSort (CirGate* i,CirGate* j) { return (i->getId() < j->getId()); }

void
CirGate::reportGate() const
{
   // e.g.
   //==================================================
   //= PO(25)”23GAT$PO”, line 9                       =
   //==================================================
   cout << "==================================================\n";
   stringstream ss;
   ss << "= " + getTypeStr() << '(' << getId() << ")";
   if(type == PI_GATE) {
      const CirPiGate* g = (CirPiGate*)this;
      if(g->getName() != "")ss << "\"" << g->getName() << "\"";
   } else if(type == PO_GATE) {
      const CirPoGate* g = (CirPoGate*)this;
      if(g->getName() != "")ss << "\"" << g->getName() << "\"";
   }
   ss << ", line " << getLineNo();
   cout << setw(49) << left << ss.str() << "=\n";
   cout << "==================================================\n";
}
void
CirGate::faninDfsVisit(int l, bool inv) const
{
   if(l == -1) return;
   for (unsigned i = 0; i < index; ++i)   cout << "  ";
   if(inv) cout << "!";
   cout << getTypeStr() << " " << id;
   unsigned size = faninList.size();
   if(l == 0) cout << endl;
   else if( isGlobalRef() ) cout << " (*)" << endl;
   else  { 
      cout << endl;
      for(unsigned i = 0; i < size; i++) {
         index++;
         getInput(i)->faninDfsVisit(l - 1, isInv(i));
      }
      if(size != 0) setToGlobalRef();
   }
   index--;
}

void
CirGate::reportFanin(int level) const
{ 
   assert (level >= 0);
   setGlobalRef();
   faninDfsVisit(level, 0);
   index = 0;
}
void
CirGate::fanoutDfsVisit(int l, bool inv) const
{
   if(l == -1) return;
   for (unsigned i = 0; i < index; ++i)   cout << "  ";
   if(inv) cout << "!";
   cout << getTypeStr() << " " << id;
   unsigned size = fanoutList.size();
   if(l == 0) cout << endl;
   else if(isGlobalRef()) cout << " (*)" << endl;
   else {
      
      vector<CirGate*> order;
      for(unsigned i = 0; i < size; i++){ 
         CirGate* g = getOutput(i);
         order.push_back(g);
      }
      std::sort (order.begin(), order.end(), orderSort);

      cout << endl;
      for(unsigned i = 0; i < size; i++) {
         index++;
         CirGate* g = order[i];
         bool myinv = 0;
         for(unsigned j = 0; j < g->getfinSize(); j++) {
            CirGate* g2 = g->getInput(j);
            if(g2 == 0) break;
            else if(g2 == this) {
               myinv = g->isInv(j);
               g->fanoutDfsVisit(l - 1, myinv);
               break; //found
            }
         }
      }
      if(size != 0) setToGlobalRef();
   }
   index--;
}

void
CirGate::reportFanout(int level) const
{
   assert (level >= 0);
   setGlobalRef();
   fanoutDfsVisit(level, 0);
   index = 0;
}

void
CirPiGate::printGate() const
{
   //[9] PI  7 (7GAT)
   //for(int i = 0; i < faninList.size(); i++)
   //   faninList[i]->printGate();
   if(isGlobalRef()) return;
   cout << "[" << index++ << "] " << setw(4) << left << getTypeStr()
      << getId();
   if(!getName().empty())
      cout << " (" << getName() << ")" << endl;
   else
      cout << endl;
   setToGlobalRef();
}
void
CirPoGate::printGate() const
{
   //[8] PO  24 !22 (22GAT$PO)
   if(isGlobalRef()) return;
   for(unsigned i = 0; i < faninList.size(); i++)
      getInput(i)->printGate();
   cout << "[" << index++ << "] " << setw(4) << left << getTypeStr() << getId() << " ";
   CirGate* fin = getInput(0);
   string str;
   if(fin->getType() == UNDEF_GATE)
      str += "*";
   if(isInv(0)) str += "!";
   str += unsToStr( fin->getId() );
   cout << str;

   if(!getName().empty())
      cout << " (" << getName() << ")" << endl;
   else
      cout << endl;
   setToGlobalRef();
}
void
CirAigGate::printGate() const
{
   //[7] AIG 22 !10 !16
   if(isGlobalRef()) return;
   for(unsigned i = 0; i < faninList.size(); i++)
      getInput(i)->printGate();
   cout << "[" << index++ << "] " << setw(4) << left << getTypeStr() << getId() << " ";
   CirGate* rhs0 = getInput(0);
   CirGate* rhs1 = getInput(1);
    
   if(rhs0 == NULL || rhs1 == NULL) return; //error
   string str;
   if(rhs0->getType() == UNDEF_GATE)
      str += "*";
   if(isInv(0)) str += "!";
   str += unsToStr( rhs0->getId() );
   cout << str << " ";
   str.clear();
   if(rhs1->getType() == UNDEF_GATE)
      str += "*";
   if(isInv(1)) str += "!";
   str += unsToStr( rhs1->getId() );
   cout << str;
   cout << endl;
   setToGlobalRef();
}
void
CirConstGate::printGate() const
{
   //[1] CONST0
   //for(int i = 0; i < faninList.size(); i++)
   //   faninList[i]->printGate();
   if(isGlobalRef()) return;
   cout << "[" << index++ << "] " << getTypeStr();
   cout << getId() << endl;
   setToGlobalRef();
}
void
CirUndefGate::printGate() const
{
   // will not be called
}

