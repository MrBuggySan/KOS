#include "include/syscalls.h"
#include "machine/CPU.h"
#include <iostream>
int localSummation(int a, int b){
  return a + b;
}
int main(){
  int a = 4;
  int b = 7;
  mword tscTemp;
  tscTemp = CPU::readTSC();
  int localResult = localSummation(a, b);
  mword localTime = CPU::readTSC() - tscTemp;
  tscTemp = CPU::readTSC();
  int syscallResult = syscallSummation(a, b);
  mword syscallTime = CPU::readTSC() -  tscTemp;
  cout << "Local: " << a << "+" << b << "=" << localResult << " (" << localTime << "cycles)"  << endl;
  cout << "Syscall: " << a << "+" << b << "="  << syscallResult << " ("  << syscallTime << "cycles)"  << endl;
    return 0;
}
