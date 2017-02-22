/******************************************************************************
    Copyright � 2012-2015 Martin Karsten

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/
#include "runtime/Thread.h"
#include "kernel/AddressSpace.h"
#include "kernel/Clock.h"
#include "kernel/Output.h"
#include "world/Access.h"
#include "machine/Machine.h"
#include "devices/Keyboard.h"
#include "runtime/Scheduler.h"

#include "main/UserMain.h"

#include <fstream>

AddressSpace kernelSpace(true); // AddressSpace.h
volatile mword Clock::tick;     // Clock.h

extern Keyboard keyboard;

#if TESTING_KEYCODE_LOOP
static void keybLoop() {
  for (;;) {
    Keyboard::KeyCode c = keyboard.read();
    StdErr.print(' ', FmtHex(c));
  }
}
#endif

// Does this handle all the edge cases? Who knows...
// Better make sure you test it thoroughly if you write it yourself!
bool String2Int(const std::string& str, mword& result)
{
  std::string::const_iterator i = str.begin();

  if (i == str.end())
      return false;

  bool negative = false;

  if (*i == '-')
  {
      negative = true;
      ++i;

      if (i == str.end())
          return false;
  }

  result = 0;

  for (; i != str.end(); ++i)
  {
      if (*i < '0' || *i > '9')
          return false;

      result *= 10;
      result += *i - '0';
  }

  if (negative)
  {
      result = -result;
  }

  return true;
}


void kosMain() {

//TODO: Print out the frequency of RTC here
  KOUT::outl("Welcome to KOS!", kendl);
  auto iter = kernelFS.find("motb");
  if (iter == kernelFS.end()) {
    KOUT::outl("motb information not found");
  } else {
    FileAccess f(iter->second);
    for (;;) {
      char c;
      if (f.read(&c, 1) == 0) break;
      KOUT::out1(c);
    }
    KOUT::outl();
  }


//Print out the contents of schedParam
mword minGranularity = 0;
mword epochLength = 0;
  iter = kernelFS.find("schedParam");

   if (iter == kernelFS.end()) {
    KOUT::outl("schedParam information not found");
  } else {
    FileAccess f(iter->second);
    bool startRead=false;
    //I'm assuming that there can only be 3 digits max on schedParam.
    std::string num;
    for (;;) {
      char c;
      if (f.read(&c, 1) == 0) break;
      if(c == ';'){
        startRead=false;
        //convert the chars into mword
        if(minGranularity == 0){
            std::string msg = "\n1st read value:";
            KOUT::out1(msg);
            if(String2Int(num, minGranularity)){
                KOUT::out1(minGranularity);
                KOUT::out1("\n");
            }else{
              KOUT::out1("fail!");
            }
            num.clear();
          //enter the value to the Scheduler object
          Scheduler::minGranularity = minGranularity;
        }else{
          std::string msg = "\n2nd read value:";
          KOUT::out1(msg);
          if(String2Int(num, epochLength)){
              KOUT::out1(epochLength);
              KOUT::out1("\n");
          }else{
            KOUT::out1("fail!");
          }
          num.clear();
          //enter the value to the Scheduler object
          Scheduler::defaultEpochLength = epochLength;
        }
      }
      if(startRead){
        std::string temp;
        temp.push_back(c);
        num+= temp;
        continue;
      }
      if(c == '='){
        startRead = true;
        num.clear();
      }

      KOUT::out1(c);
    }
    KOUT::outl();
  }




#if TESTING_TIMER_TEST
  StdErr.print(" timer test, 3 secs...");
  for (int i = 0; i < 3; i++) {
    Timeout::sleep(Clock::now() + 1000);
    StdErr.print(' ', i+1);
  }
  StdErr.print(" done.", kendl);
#endif
#if TESTING_KEYCODE_LOOP
  Thread* t = Thread::create()->setPriority(topPriority);
  Machine::setAffinity(*t, 0);
  t->start((ptr_t)keybLoop);
#endif
  Thread::create()->start((ptr_t)UserMain);
#if TESTING_PING_LOOP
  for (;;) {
    Timeout::sleep(Clock::now() + 1000);
    KOUT::outl("...ping...");
  }
#endif
}

extern "C" void kmain(mword magic, mword addr, mword idx)         __section(".boot.text");
extern "C" void kmain(mword magic, mword addr, mword idx) {
  if (magic == 0 && addr == 0xE85250D6) {
    // low-level machine-dependent initialization on AP
    Machine::initAP(idx);
  } else {
    // low-level machine-dependent initialization on BSP -> starts kosMain
    Machine::initBSP(magic, addr, idx);
  }
}
