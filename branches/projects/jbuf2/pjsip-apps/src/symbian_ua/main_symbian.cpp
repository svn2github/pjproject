/* $Id$ */
/* 
 * Copyright (C) 2003-2007 Benny Prijono <benny@prijono.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */
#include "ua.h"

#include <e32std.h>
#include <e32base.h>
#include <e32std.h>
#include <stdlib.h>


//  Global Variables
CConsoleBase* console;


////////////////////////////////////////////////////////////////////////////
class MyTask : public CActive
{
public:
    static MyTask *NewL(CActiveSchedulerWait *asw);
    ~MyTask();
    void Start();

protected:
    MyTask(CActiveSchedulerWait *asw);
    void ConstructL();
    virtual void RunL();
    virtual void DoCancel();

private:
    RTimer timer_;
    CActiveSchedulerWait *asw_;
};

MyTask::MyTask(CActiveSchedulerWait *asw)
: CActive(EPriorityNormal), asw_(asw)
{
}

MyTask::~MyTask() 
{
    timer_.Close();
}

void MyTask::ConstructL()
{
    timer_.CreateLocal();
    CActiveScheduler::Add(this);
}

MyTask *MyTask::NewL(CActiveSchedulerWait *asw)
{
    MyTask *self = new (ELeave) MyTask(asw);
    CleanupStack::PushL(self);

    self->ConstructL();

    CleanupStack::Pop(self);
    return self;
}

void MyTask::Start()
{
    timer_.After(iStatus, 0);
    SetActive();
}

void MyTask::RunL()
{
    ua_main();
    asw_->AsyncStop();
}

void MyTask::DoCancel()
{

}

////////////////////////////////////////////////////////////////////////////

LOCAL_C void DoStartL()
{
    CActiveScheduler *scheduler = new (ELeave) CActiveScheduler;
    CleanupStack::PushL(scheduler);
    CActiveScheduler::Install(scheduler);

    CActiveSchedulerWait *asw = new CActiveSchedulerWait;
    CleanupStack::PushL(asw);
    
    MyTask *task = MyTask::NewL(asw);
    task->Start();

    asw->Start();
    
    delete task;
    
    CleanupStack::Pop(asw);
    delete asw;
    
    CActiveScheduler::Install(NULL);
    CleanupStack::Pop(scheduler);
    delete scheduler;
}


////////////////////////////////////////////////////////////////////////////

// E32Main()
GLDEF_C TInt E32Main()
{
    // Mark heap usage
    __UHEAP_MARK;

    // Create cleanup stack
    CTrapCleanup* cleanup = CTrapCleanup::New();

    // Create output console
    TRAPD(createError, console = Console::NewL(_L("Console"), TSize(KConsFullScreen,KConsFullScreen)));
    if (createError)
        return createError;

    TRAPD(startError, DoStartL());

    console->Printf(_L("[press any key to close]\n"));
    console->Getch();
    
    delete console;
    delete cleanup;

    CloseSTDLIB(); 

    // Mark end of heap usage, detect memory leaks
    __UHEAP_MARKEND;
    return KErrNone;
}

