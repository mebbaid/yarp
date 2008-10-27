// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-

/*
* Copyright (C) 2006 Lorenzo Natale
* CopyPolicy: Released under the terms of the GNU GPL v2.0.
*/

#include <yarp/sig/Matrix.h>
#include <yarp/os/impl/BufferedConnectionWriter.h>
#include <yarp/os/Bottle.h>
#include <yarp/os/Thread.h>
#include <yarp/os/Port.h>
#include <yarp/os/Time.h>

#include <yarp/gsl_compatibility.h>


#include <vector>

#include "TestList.h"

using namespace yarp::os::impl;
using namespace yarp::os;
using namespace yarp::sig;

//
class MThread1:public Thread
{
public:
    MThread1(Port *p)
    {
        portOut=p;
    }

    bool threadInit()
    {
        success=false;
        return true;
    }

    void run()
    {
        Matrix m;

        int times=10;

        while(times--)
        {
            m.resize(4,4);
            int r=0;
            int c=0;
            for(r=0; r<4; r++)
                for (c=0; c<4; c++)
                    m[r][c]=99;

            portOut->write(m);
            Time::delay(0.1);

            m.resize(2,4);
            for(r=0; r<2; r++)
                for (c=0; c<4; c++)
                    m[r][c]=66;

            portOut->write(m);
        }

        success=true;
    }

    Port *portOut;
    bool success;
};

class MThread2:public Thread
{
public:
    MThread2(Port *p)
    {
        portIn=p;
    }

    bool threadInit()
    {
        success=false;
        return true;
    }

    void run()
    {
        Matrix m;

        int times=10;
        bool ok=true;
        while(times--)
        {
            portIn->read(m);
            if ( (m.rows()!=4)||(m.cols()!=4))
                ok=false;


            portIn->read(m);

            if ( (m.rows()!=2)||(m.cols()!=4))
                ok=false;
        }

        success=ok;
    }

    Port *portIn;
    bool success;
};

class MatrixTest : public UnitTest {
    
    bool checkConsistency(Matrix &a)
    {
        gsl_matrix *tmp;
        tmp=(gsl_matrix *)(a.getGslMatrix());
        bool ret=true;
        if ((int)tmp->size1!=a.rows())
            ret=false;

        if ((int)tmp->size2!=a.cols())
            ret=false;

        if ((int)tmp->block->size!=a.cols()*a.rows())
            ret=false;

        if (tmp->data!=a.data())
            ret=false;

        if (tmp->block->data!=a.data())
            ret=false;

        return ret;
    }

public:
    virtual String getName() { return "MatrixTest"; }

    void checkGsl()
    {
        Matrix a(5,5);
        Matrix b;
        b=a;
        checkTrue(checkConsistency(a), "gsldata consistent after creation");
        checkTrue(checkConsistency(b), "gsldata consistent after copy");
        b.resize(100,100);
        checkTrue(checkConsistency(b), "gsldata consistent after resize");

        Matrix s=a.submatrix(1,1,2,2);
        checkConsistency(s);
        checkTrue(checkConsistency(s), "gsldata consistent for submatrix");
        Matrix c=a;
        checkTrue(checkConsistency(c), "gsldata consistent after init");
    }

    void checkOperators()
    {
        report(0,"checking operator ==");

        Matrix M1(3,3);
        Matrix M2(3,3);

        M1=1;
        M2=1; //now we have to identical vectors

        bool ok=false;
        if (M1==M2)
            ok=true;

        M1=2;
        M2=1; //now vectors are different
        if (M1==M2)
            ok=false;

        checkTrue(ok, "operator== for matrix work");
    }

    void checkSendReceive()
    {
        Port portIn;
        Port portOut;

        MThread2 *receiverThread=new MThread2(&portIn);
        MThread1 *senderThread=new MThread1(&portOut);

        portOut.open("/harness_sig/mtest/o");
        portIn.open("/harness_sig/mtest/i");

        Network::connect("/harness_sig/mtest/o", "/harness_sig/mtest/i");

        receiverThread->start();
        senderThread->start();

        receiverThread->stop();
        senderThread->stop();

        portOut.close();
        portIn.close();

        checkTrue(senderThread->success, "Send matrix test");
        checkTrue(receiverThread->success, "Receive matrix test");

        delete receiverThread;
        delete senderThread;
    }

    void checkCopyCtor()
    {
        report(0,"check matrix copy constructor works...");
        Matrix m(4,4);
        int r=0;
        int c=0;
        for(r=0; r<4; r++)
        {
            for (c=0; c<4; c++)
                m[r][c]=1333;
        }

        Matrix m2(m);
        checkEqual(m.rows(),m2.rows(),"rows matches");
        checkEqual(m.cols(),m2.cols(),"cols matches");

        bool ok=true;
        for(r=0; r<4; r++)
            for (c=0; c<4; c++)
                ok=ok && ((m[r])[c]==(m2[r])[c]);

        checkTrue(ok,"elements match");
    }

    void checkCopy() {
        report(0,"check matrix copy constructor works...");
        Matrix m(4,4);
        int r=0;
        int c=0;
        for(r=0; r<4; r++)
            for (c=0; c<4; c++)
                m[r][c]=99;

        Matrix m2(m);
        checkEqual(m.rows(),m2.rows(),"rows matches");
        checkEqual(m.cols(),m2.cols(),"cols matches");

        bool ok=true;
        for(r=0; r<4; r++)
            for (c=0; c<4; c++)
                ok=ok && (m[r][c]==m2[r][c]);
        checkTrue(ok,"elements match");   
    }

    void checkSubmatrix()
    {
        report(0,"check function Matrix::submatrix works...");
        const int R=10;
        const int C=20;
        Matrix m(R,C);

        int r=0;
        int c=0;
        int kk=0;
        for(r=0; r<R; r++)
            for (c=0; c<C; c++)
                m[r][c]=kk++;

        report(0,"extracting submatrix...");
        int r1=5;
        int r2=8;
        int c1=4;
        int c2=8;
        Matrix m2=m.submatrix(r1, r2, c1, c2);

        checkEqual(r2-r1+1,m2.rows(),"rows matches");
        checkEqual(c2-c1+1,m2.cols(),"cols matches");
        
        kk=r1*C+c1;
        bool ok=true;
        for(r=0; r<m2.rows(); r++)
        {
            int cc=kk;
            for(c=0;c<m2.cols();c++)
            {
                if (m2[r][c]!=cc++)
                    ok=false;
            }
            kk+=C;
        }

        checkTrue(ok,"elements match");

        report(0,"extracting full size matrix...");
        Matrix m3=m.submatrix(0, R-1, 0, C-1);
        checkEqual(R,m3.rows(),"rows matches");
        checkEqual(C,m3.cols(),"cols matches");

        kk=0;
        ok=true;
        for(r=0; r<m3.rows(); r++)
        {
            int cc=kk;
            for(c=0;c<m3.cols();c++)
            {
                if (m3[r][c]!=cc++)
                    ok=false;
            }
            kk+=C;
        }
        checkTrue(ok,"elements match");
    }

    virtual void runTests() {
        Network::setLocalMode(true);
        checkCopyCtor();
        checkCopy();
        checkSendReceive();
        checkOperators();
        checkGsl();
        Network::setLocalMode(false);
        checkSubmatrix();
    }
};

static MatrixTest theMatrixTest;

UnitTest& getMatrixTest() {
    return theMatrixTest;
}
