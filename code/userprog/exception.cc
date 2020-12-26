// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "main.h"
#include "syscall.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

void
ExceptionHandler(ExceptionType which)
{
	int	type = kernel->machine->ReadRegister(2);
	int	val;

    switch (which) {
		case SyscallException:
			switch(type) {
				case SC_Halt:
					DEBUG(dbgAddr, "Shutdown, initiated by user program.\n");
					kernel->interrupt->Halt();
					break;
				case SC_PrintInt:
					val=kernel->machine->ReadRegister(4);
					cout << "Print integer:" <<val << endl;
					return;
				/*case SC_Exec:
					DEBUG(dbgAddr, "Exec\n");
					val = kernel->machine->ReadRegister(4);
					kernel->StringCopy(tmpStr, retVal, 1024);
					cout << "Exec: " << val << endl;
					val = kernel->Exec(val);
					kernel->machine->WriteRegister(2, val);
					return;
				*/		
				case SC_Exit:
					DEBUG(dbgAddr, "Program exit\n");
					val=kernel->machine->ReadRegister(4);
					cout << "return value:" << val << endl;
					kernel->currentThread->Finish();
					break;
				default:
					cerr << "Unexpected system call " << type << "\n";
					break;
			}
			break;
		case PageFaultException:
			cout << "page fault" << endl;

			//count virtual page number
			unsigned int vpn = (unsigned)machine->ReadRegister(PCReg)/ PageSize;
			//get the missing page
			TranslationEntry *missingPage = machine->pageTable[vpn];

			//find empty physical memory page
			int j = 0;
			while(j < NumPhysPages && AddrSpace::usedPhyPage[j] == true) j++;

			//read page from virtual memory
			char *buf = new char[PageSize];
			kernel->virtualMemory->ReadSector(missingPage->virtualMemPage, buf);


			//Physical memory is enough, swap page in main memory
			if(j != 32)
			{
				missingPage->physicalPage = j;
				missingPage->valid = true;
				
				AddrSpace::usedPhyPage[j] = true;
				AddrSpace::usedVirPage[missingPage->virtualMemPage] = false;
				//load missing page in main memory
				bcopy(buf, &machine->mainMemory[j * pageSize], pageSize);
			}
			//Physical memory isn't enough, page replacement occur
			else
			{
				char *buf2 = new char[PageSize];
				if(kernel::pra == pageReplacementAlgor::FIFO)
				{
					unsigned int phyPageVic = AddrSpace::orderOfPages.RemoveFront(); // find the earliest used page
					TranslationEntry *victim = AddrSpace::invertedTable[phyPageVic]; //find the victim
					AddrSpace::invertedTable[phyPageVic] = missingPage; 


					missingPage->physicalPage = phyPageVic;
					missingPage->valid = true;

					victim->virtualMemPage = missingPage->virtualMemPage;
					victim->valid = false;

					//read victim's data and write in virtual memory
					bcopy(&machine->mainMemory[phyPageVic * pageSize] , buf2, pageSize);
					kernel->virtualMemory->WriteSector(missingPage->virtualMemPage, buf2);

					//load missing page in main memory
					bcopy(buf, &machine->mainMemory[phyPageVic * pageSize], pageSize);
				}
				else if(kernel::pra == pageReplacementAlgor::LRU)
				{

				}
				delete buf2;
			}
			delete buf;
			break;
		default:
			cerr << "Unexpected user mode exception" << which << "\n";
			break;
    }
    ASSERTNOTREACHED();
}
