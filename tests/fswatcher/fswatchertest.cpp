///////////////////////////////////////////////////////////////////////////////
// Name:        tests/fswatcher/fswatchertest.cpp
// Purpose:     wxFileSystemWatcher unit test
// Author:      Bartosz Bekier
// Created:     2009-06-11
// RCS-ID:      $Id$
// Copyright:   (c) 2009 Bartosz Bekier
///////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "testprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include <cstdlib>
#include <iostream>
#include "wx/evtloop.h"
#include "wx/filename.h"
#include "wx/filefn.h"
#include "wx/stdpaths.h"
#include "wx/fswatcher.h"

// ----------------------------------------------------------------------------
// local functions
// ----------------------------------------------------------------------------
#include <ostream>
using namespace std;

// helper to print wxFileName. needed by CPPUNIT_ASSERT_EQUAL
extern ostream& operator<<(ostream& out, const wxFileName& fn);
//{
//    return out << fn.GetFullPath();
//}

// class generating file system events
class EventGenerator
{
public:
    static EventGenerator& Get()
    {
        if (!ms_instance)
            ms_instance = new EventGenerator(GetWatchDir());

        return *ms_instance;
    }

    EventGenerator(const wxFileName& path) : m_base(path)
    {
        m_old = wxFileName();
        m_file = RandomName();
        m_new = RandomName();
    }

    // operations
    bool CreateFile()
    {
        wxFile file(m_file.GetFullPath(), wxFile::write);
        return file.IsOpened() && m_file.FileExists();
    }

    bool RenameFile()
    {
        CPPUNIT_ASSERT(m_file.FileExists());

        wxLogDebug("Renaming %s=>%s", m_file.GetFullPath(), m_new.GetFullPath());

        bool ret = wxRenameFile(m_file.GetFullPath(), m_new.GetFullPath());
        if (ret)
        {
            m_old = m_file;
            m_file = m_new;
            m_new = RandomName();
        }

        return ret;
    }

    bool DeleteFile()
    {
        CPPUNIT_ASSERT(m_file.FileExists());

        bool ret =  wxRemoveFile(m_file.GetFullPath());
        if (ret)
        {
            m_old = m_file;
            m_file = m_new;
            m_new = RandomName();
        }

        return ret;
    }

    bool TouchFile()
    {
        return m_file.Touch();
    }

    bool ReadFile()
    {
        wxFile f(m_file.GetFullPath());
        CPPUNIT_ASSERT(f.IsOpened());

        char buf[1];
        ssize_t count = f.Read(buf, sizeof(buf));
        CPPUNIT_ASSERT(count > 0);

    	return true;
    }

    bool ModifyFile()
    {
        CPPUNIT_ASSERT(m_file.FileExists());

        wxFile file(m_file.GetFullPath(), wxFile::write_append);
        CPPUNIT_ASSERT(file.IsOpened());

        CPPUNIT_ASSERT(file.Write("Words of Wisdom, Lloyd. Words of wisdom\n"));
        return file.Close();
    }

    // helpers
    wxFileName RandomName(int length = 10)
    {
        return RandomName(m_base, length);
    }

    // static helpers
    static const wxFileName& GetWatchDir()
    {
        static wxFileName dir;

        if (dir.DirExists())
            return dir;

        wxString tmp = wxStandardPaths::Get().GetTempDir();
        dir.AssignDir(tmp);

        // XXX look for more unique name? there is no function to generate
        // unique filename, the file always get created...
        dir.AppendDir("fswatcher_test");
        CPPUNIT_ASSERT(!dir.DirExists());
        CPPUNIT_ASSERT(dir.Mkdir());

        return dir;
    }

    static void RemoveWatchDir()
    {
        wxFileName dir = GetWatchDir();
        CPPUNIT_ASSERT(dir.DirExists());

        // just to be really sure we know what we remove
        CPPUNIT_ASSERT(dir.GetDirs().Last() == "fswatcher_test");
        CPPUNIT_ASSERT(dir.Rmdir(wxPATH_RMDIR_RECURSIVE));
    }

    static wxFileName RandomName(const wxFileName& base, int length = 10)
    {
        static int ALFA_CNT = 'z' - 'a';

        wxString s;
        int i = 0;
        for ( ; i < length; ++i)
        {
            char c = 'a' + (rand() % ALFA_CNT);

            // XXX when done this way it doesn't work with wxFileName ctor!
            // s[i] = c;
            s += c;
        }

        return wxFileName(base.GetFullPath(), s);
    }

public:
    wxFileName m_base;     // base dir for doing operations
    wxFileName m_file;     // current file name
    wxFileName m_old;      // previous file name
    wxFileName m_new;      // name after renaming

protected:
    static EventGenerator* ms_instance;
};

EventGenerator* EventGenerator::ms_instance = 0;


// custom event handler
class EventHandler : public wxEvtHandler
{
public:
    const static int WAIT_DURATION = 3;

    EventHandler() :
        eg(EventGenerator::Get()), m_loop(0), m_count(0), m_watcher(0)
    {
        m_loop = new wxEventLoop();
        Bind(wxEVT_IDLE, &EventHandler::OnIdle, this);
        Bind(wxEVT_FSWATCHER, &EventHandler::OnFileSystemEvent, this);
    }

    virtual ~EventHandler()
    {
        if (m_watcher)
            delete m_watcher;
        if (m_loop) {
            if (m_loop->IsRunning())
                m_loop->Exit();
            delete m_loop;
        }

        // XXX we need this hack, because it gets messy with every other loop
    }

    void Exit()
    {
        // needed here to unregister source from loop before destroying it
//        if (m_watcher)
//        {
//            delete m_watcher;
//            m_watcher = 0;
//        }

        m_loop->Exit();
    }

    // sends idle event, so we get called in a moment
    void SendIdle()
    {
        wxIdleEvent* e = new wxIdleEvent();
        QueueEvent(e);
    }

    void Run()
    {
        SendIdle();
        m_loop->Run();
    }

    void OnIdle(wxIdleEvent& /*evt*/)
    {
//      wxLogDebug(wxString::Format("--- OnIdle %d ---", m_count));

        bool more = Action();
        m_count++;

        if (more)
        {
            SendIdle();
        }
    }

    // returns whether we should produce more idle events
    virtual bool Action()
    {
        switch (m_count)
        {
        case 0:
            CPPUNIT_ASSERT(Init());
            break;
        case 1:
            CPPUNIT_ASSERT(GenerateEvent());
            break;
        case 2:
            // actual test
    		CPPUNIT_ASSERT(CheckResult());
    		Exit();
    		break;

        // TODO a mechanism that will break the loop in case we
        // don't receive a file system event
        // this below doesn't quite work, so all tests must pass :-)
#if 0
        case 2:
            m_loop.Yield();
            m_loop.WakeUp();
            CPPUNIT_ASSERT(KeepWaiting());
            m_loop.Yield();
            break;
        case 3:
            break;
        case 4:
            CPPUNIT_ASSERT(AfterWait());
            break;
#endif
        } // switch (m_count)

        return m_count <= 0;
    }

    virtual bool Init()
    {
        // test we're good to go
        CPPUNIT_ASSERT(wxEventLoopBase::GetActive());

        // XXX only now can we construct Watcher, because we need
        // active loop here
        m_watcher = new wxFileSystemWatcher();
        m_watcher->SetOwner(this);

        // add dir to be watched
        wxFileName dir = EventGenerator::GetWatchDir();
        CPPUNIT_ASSERT(m_watcher->Add(dir, wxFSW_EVENT_ALL));

        return true;
    }

    virtual bool KeepWaiting()
    {
        // did we receive event already?
        if (!tested)
        {
            // well, lets wait a bit more
            wxSleep(WAIT_DURATION);
        }

        return true;
    }

    virtual bool AfterWait()
    {
        // fail if still no events
         if (!tested)
         {
             wxString s;
             s.Printf("No events from watcher during %d seconds!",
                                                             WAIT_DURATION);
             CPPUNIT_FAIL((const char*)s);
         }

         return true;
    }

    virtual void OnFileSystemEvent(wxFileSystemWatcherEvent& evt)
    {
        wxLogDebug("--- %s ---", evt.ToString());
        m_lastEvent = wxDynamicCast(evt.Clone(), wxFileSystemWatcherEvent);
        m_events.Add(m_lastEvent);

        // test finished
        SendIdle();
        tested = true;
    }

    virtual bool CheckResult()
    {
        // have any events?
        CPPUNIT_ASSERT(m_events.size() > 0);

        // this is our "reference event"
        wxFileSystemWatcherEvent expected = ExpectedEvent();

        // have event of expected type?
        wxFileSystemWatcherEvent* e = 0;
        wxArrayEvent::iterator it = m_events.begin();
        for ( ; it != m_events.end(); ++it)
        {
            if ((*it)->GetChangeType() == expected.GetChangeType())
            {
                e = *it;
                break;
            }
        }
        CPPUNIT_ASSERT(e);

        // ok, lets check that event is correct
        CPPUNIT_ASSERT_EQUAL((int)wxEVT_FSWATCHER, e->GetEventType());

        // XXX this needs change
        CPPUNIT_ASSERT_EQUAL(wxEVT_CATEGORY_UNKNOWN, e->GetEventCategory());

        CPPUNIT_ASSERT_EQUAL(expected.GetPath(), e->GetPath());
        CPPUNIT_ASSERT_EQUAL(expected.GetNewPath(), e->GetNewPath());
        CPPUNIT_ASSERT_EQUAL(expected.GetChangeType(), e->GetChangeType());

        return true;
    }

    virtual bool GenerateEvent() = 0;

    virtual wxFileSystemWatcherEvent ExpectedEvent() = 0;


protected:
    EventGenerator& eg;
    wxEventLoopBase* m_loop;    // loop reference
    int m_count;                // idle events count

    wxFileSystemWatcher* m_watcher;
    bool tested;  // indicates, whether we have already passed the test

    #include "wx/arrimpl.cpp"
    WX_DEFINE_ARRAY_PTR(wxFileSystemWatcherEvent*, wxArrayEvent);
    wxArrayEvent m_events;
    wxFileSystemWatcherEvent* m_lastEvent;
};


// ----------------------------------------------------------------------------
// test class
// ----------------------------------------------------------------------------

class FileSystemWatcherTestCase : public CppUnit::TestCase
{
public:
    FileSystemWatcherTestCase() { }

    virtual void setUp();
    virtual void tearDown();

protected:
    wxEventLoopBase* m_loop;

private:
    CPPUNIT_TEST_SUITE( FileSystemWatcherTestCase );
        CPPUNIT_TEST( TestEventCreate );
        CPPUNIT_TEST( TestEventDelete );
        CPPUNIT_TEST( TestEventRename );
        CPPUNIT_TEST( TestEventModify );
        CPPUNIT_TEST( TestEventAccess );
    CPPUNIT_TEST_SUITE_END();

    void TestEventCreate();
    void TestEventDelete();
    void TestEventRename();
    void TestEventModify();
    void TestEventAccess();

    DECLARE_NO_COPY_CLASS(FileSystemWatcherTestCase)
};

// register in the unnamed registry so that these tests are run by default
CPPUNIT_TEST_SUITE_REGISTRATION( FileSystemWatcherTestCase );

// also include in it's own registry so that these tests can be run alone
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION( FileSystemWatcherTestCase,
                                        "FileSystemWatcherTestCase" );

void FileSystemWatcherTestCase::setUp()
{
    EventGenerator::Get().GetWatchDir();
}

void FileSystemWatcherTestCase::tearDown()
{
    EventGenerator::Get().RemoveWatchDir();
}

// ----------------------------------------------------------------------------
// TestEventCreate
// ----------------------------------------------------------------------------
void FileSystemWatcherTestCase::TestEventCreate()
{
    wxLogDebug("TestEventCreate()");

    class EventTester : public EventHandler
    {
    public:
        virtual bool GenerateEvent()
        {
            CPPUNIT_ASSERT(eg.CreateFile());
            return true;
        }

        virtual wxFileSystemWatcherEvent ExpectedEvent()
        {
            wxFileSystemWatcherEvent event(wxFSW_EVENT_CREATE);
            event.SetPath(eg.m_file);
            event.SetNewPath(eg.m_file);
            return event;
        }
    };

    EventTester tester;

    wxLogTrace(wxTRACE_FSWATCHER, "TestEventCreate tester created()");

    tester.Run();
}

// ----------------------------------------------------------------------------
// TestEventDelete
// ----------------------------------------------------------------------------
void FileSystemWatcherTestCase::TestEventDelete()
{
    wxLogDebug("TestEventDelete()");

    class EventTester : public EventHandler
    {
    public:
        virtual bool GenerateEvent()
        {
            CPPUNIT_ASSERT(eg.DeleteFile());
            return true;
        }

        virtual wxFileSystemWatcherEvent ExpectedEvent()
        {
            wxFileSystemWatcherEvent event(wxFSW_EVENT_DELETE);
            event.SetPath(eg.m_old);

            // CHECK maybe new path here could be NULL or sth?
            event.SetNewPath(eg.m_old);
            return event;
        }
    };

    // we need to create a file now, so we can delete it
    EventGenerator::Get().CreateFile();

    EventTester tester;
    tester.Run();
}

// ----------------------------------------------------------------------------
// TestEventRename
// ----------------------------------------------------------------------------
void FileSystemWatcherTestCase::TestEventRename()
{
    wxLogDebug("TestEventRename()");

    class EventTester : public EventHandler
    {
    public:
        virtual bool GenerateEvent()
        {
            CPPUNIT_ASSERT(eg.RenameFile());
            return true;
        }

        virtual wxFileSystemWatcherEvent ExpectedEvent()
        {
            wxFileSystemWatcherEvent event(wxFSW_EVENT_RENAME);
            event.SetPath(eg.m_old);
            event.SetNewPath(eg.m_file);
            return event;
        }
    };

    // need a file to rename later
    EventGenerator::Get().CreateFile();

    EventTester tester;
    tester.Run();
}

// ----------------------------------------------------------------------------
// TestEventModify
// ----------------------------------------------------------------------------
void FileSystemWatcherTestCase::TestEventModify()
{
    wxLogDebug("TestEventModify()");

    class EventTester : public EventHandler
    {
    public:
        virtual bool GenerateEvent()
        {
            CPPUNIT_ASSERT(eg.ModifyFile());
            return true;
        }

        virtual wxFileSystemWatcherEvent ExpectedEvent()
        {
            wxFileSystemWatcherEvent event(wxFSW_EVENT_MODIFY);
            event.SetPath(eg.m_file);
            event.SetNewPath(eg.m_file);
            return event;
        }
    };

    // we need to create a file to modify
    EventGenerator::Get().CreateFile();

    EventTester tester;
    tester.Run();
}

// ----------------------------------------------------------------------------
// TestEventAccess
// ----------------------------------------------------------------------------
void FileSystemWatcherTestCase::TestEventAccess()
{
    wxLogDebug("TestEventAccess()");

    class EventTester : public EventHandler
    {
    public:
        virtual bool GenerateEvent()
        {
            // funny, ReadFile generates ACCESS in inotify and under MSW
            // it does not... Touch generates MODIFY, but at least the test
            // doesn't pass for now instead of hanging
            CPPUNIT_ASSERT(eg.TouchFile());

//            CPPUNIT_ASSERT(eg.ReadFile());
            return true;
        }

        virtual wxFileSystemWatcherEvent ExpectedEvent()
        {
            wxFileSystemWatcherEvent event(wxFSW_EVENT_ACCESS);
            event.SetPath(eg.m_file);
            event.SetNewPath(eg.m_file);
            return event;
        }
    };

    // we need to create a file to read from it and write sth to it
    EventGenerator::Get().CreateFile();
    EventGenerator::Get().ModifyFile();

    EventTester tester;
    tester.Run();
}
