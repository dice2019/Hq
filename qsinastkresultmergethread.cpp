#include "qsinastkresultmergethread.h"
#include <QtConcurrent>
#include <QFuture>
#include "qsinastkinfothread.h"
#include "profiles.h"
#include <QDebug>
#include "qexchangedatamanage.h"

QSinaStkResultMergeThread::QSinaStkResultMergeThread(QObject *parent) : QThread(parent)
{
    mCodeChangeFlag = false;
    mStkCntPerTrd = 200;
    mTotalPage = -1;
    mPageSize = 50;
    mCurPage = 1;
    mMktType = MKT_ALL;
    mSortType = STK_DISPLAY_SORT_TYPE_CHGPER;
    mSortRule = -1;
    mActive = true;
    QEastMoneyZjlxThread *zjt = new QEastMoneyZjlxThread(this);
    connect(zjt, SIGNAL(sendZjlxDataList(QList<zjlxData>)), this, SLOT(slotRevZjlxData(QList<zjlxData>)));
    zjt->start();
}

QSinaStkResultMergeThread::~QSinaStkResultMergeThread()
{

}

void QSinaStkResultMergeThread::setActive(bool active)
{
    mActive = active;
}

bool QSinaStkResultMergeThread::isActive()
{
    return mActive;
}

void QSinaStkResultMergeThread::setSortType(int type)
{
    if(mSortType == type)
    {
        mSortRule = (mSortRule == -1 ? 1: -1);

    } else
    {
        mSortType = type;
        mSortRule = -1;
    }
}

void QSinaStkResultMergeThread::run()
{
    while (true) {
        //检查当前的代码是否发生了变化
        if(mStkCodesList.length() == 0)
        {
            sleep(1);
            continue;
        }
        if(mThreadList.length() == 0)
        {
            //还没有初始化行情线程
            int thread_code = 50;
            int nthread = (mStkCodesList.length() + thread_code-1 ) / thread_code;
            for(int i=0; i<nthread; i++)
            {
                QStringList wklist = mStkCodesList.mid(i*thread_code, thread_code);
                QSinaStkInfoThread *wkthread = new QSinaStkInfoThread();
                mThreadList.append(wkthread);
                wkthread->setOptType(STK_DISPLAY_SORT_TYPE_NONE);
                wkthread->setStkList(wklist);
                connect(wkthread, SIGNAL(sendStkDataList(StockDataList)), this, SLOT(slotRevResList(StockDataList)));
                wkthread->start();
            }
        }
        StockDataList wklist;
        mListMutex.lock();
        if(mMktType == MKT_ALL)
        {
            //全部进行排序，不过滤
            wklist = mMidStkDataMapList.values();
        } else if(mMktType == MKT_SH)
        {
            foreach (QString key, mMidStkDataMapList.keys()) {
                if(key.left(1) == "6" || key.left(1) == "5")
                {
                    wklist.append(mMidStkDataMapList.value(key));
                }
            }

        }else if(mMktType == MKT_SZ)
        {
            foreach (QString key, mMidStkDataMapList.keys()) {
                if(key.left(1) == "0" || key.left(1) == "3")
                    wklist.append(mMidStkDataMapList.value(key));
            }
        }
        else if(mMktType == MKT_ZXB)
        {
            foreach (QString key, mMidStkDataMapList.keys()) {
                if(key.left(3) == "002")
                    wklist.append(mMidStkDataMapList.value(key));
            }

        }
        else if(mMktType == MKT_CYB)
        {
            foreach (QString key, mMidStkDataMapList.keys()) {
                if(key.left(1) == "3")
                {
                    wklist.append(mMidStkDataMapList.value(key));
                }
            }

        } else if(mMktType == MKT_ZXG || mMktType == MKT_OTHER)
        {
            foreach (QString key, mSelfCodesList) {
                if(mMidStkDataMapList.contains(key.right(6)))
                    wklist.append(mMidStkDataMapList.value(key.right(6)));
            }

            //qDebug()<<"wklist len"<<wklist.length();
        }
        mListMutex.unlock();
        mTotalPage = (wklist.length() + mPageSize -1) / mPageSize;

        if(wklist.length())
        {
            //开始排序
            if(mSortType == STK_DISPLAY_SORT_TYPE_CHGPER)
            {
                if(mSortRule == -1)
                {
                    qSort(wklist.begin(), wklist.end(), StockData::sortByPerDesc);
                } else
                {
                    qSort(wklist.begin(), wklist.end(), StockData::sortByPerAsc);
                }
            }

            else if(mSortType == STK_DISPLAY_SORT_TYPE_CJE)
            {
                if(mSortRule == -1)
                {
                    qSort(wklist.begin(), wklist.end(), StockData::sortByMonDesc);
                } else
                {
                    qSort(wklist.begin(), wklist.end(), StockData::sortByMonAsc);
                }
            }

            else if(mSortType == STK_DISPLAY_SORT_TYPE_PRICE)
            {
                if(mSortRule == -1)
                {
                    qSort(wklist.begin(), wklist.end(), StockData::sortByCurDesc);
                } else
                {
                    qSort(wklist.begin(), wklist.end(), StockData::sortByCurAsc);
                }
            }else if(mSortType == STK_DISPLAY_SORT_TYPE_HSL)
            {
                if(mSortRule == -1)
                {
                    qSort(wklist.begin(), wklist.end(), StockData::sortByHslDesc);
                } else
                {
                    qSort(wklist.begin(), wklist.end(), StockData::sortByHslAsc);
                }
            }else if(mSortType == STK_DISPLAY_SORT_TYPE_MONEYR)
            {
                if(mSortRule == -1)
                {
                    qSort(wklist.begin(), wklist.end(), StockData::sortByMonRatioDesc);
                } else
                {
                    qSort(wklist.begin(), wklist.end(), StockData::sortByMonRatioAsc);
                }
            }
            else if(mSortType == STK_DISPLAY_SORT_TYPE_LAST3)
            {
                if(mSortRule == -1)
                {
                    qSort(wklist.begin(), wklist.end(), StockData::sortBy3DayChgDesc);
                } else
                {
                    qSort(wklist.begin(), wklist.end(), StockData::sortBy3DayChgAsc);
                }
            }

            else if(mSortType == STK_DISPLAY_SORT_TYPE_ZJLX)
            {
                if(mSortRule == -1)
                {
                    qSort(wklist.begin(), wklist.end(), StockData::sortBy3DayZjlxDesc);
                } else
                {
                    qSort(wklist.begin(), wklist.end(), StockData::sortBy3DayZjlxAsc);
                }
            }
            else if(mSortType == STK_DISPLAY_SORT_TYPE_GXL)
            {
                if(mSortRule == -1)
                {
                    qSort(wklist.begin(), wklist.end(), StockData::sortByGxlDesc);
                } else
                {
                    qSort(wklist.begin(), wklist.end(), StockData::sortByGxlAsc);
                }
            }
            else if(mSortType == STK_DISPLAY_SORT_TYPE_SZZBL)
            {
                if(mSortRule == -1)
                {
                    qSort(wklist.begin(), wklist.end(), StockData::sortBySzzblDesc);
                } else
                {
                    qSort(wklist.begin(), wklist.end(), StockData::sortBySzzblAsc);
                }
            }
            else if(mSortType == STK_DISPLAY_SORT_TYPE_GQDJR)
            {
                if(mSortRule == -1)
                {
                    qSort(wklist.begin(), wklist.end(), StockData::sortByGqdjrDesc);
                } else
                {
                    qSort(wklist.begin(), wklist.end(), StockData::sortByGqdjrAsc);
                }
            }
            else if(mSortType == STK_DISPLAY_SORT_TYPE_TCAP)
            {
                if(mSortRule == -1)
                {
                    qSort(wklist.begin(), wklist.end(), StockData::sortByTcapDesc);
                } else
                {
                    qSort(wklist.begin(), wklist.end(), StockData::sortByTcapAsc);
                }
            }
            else if(mSortType == STK_DISPLAY_SORT_TYPE_MCAP)
            {
                if(mSortRule == -1)
                {
                    qSort(wklist.begin(), wklist.end(), StockData::sortByMcapDesc);
                } else
                {
                    qSort(wklist.begin(), wklist.end(), StockData::sortByMcapAsc);
                }
            }
            if(mActive)emit sendStkDataList(wklist.mid((mCurPage - 1) * mPageSize, mPageSize));
        } else
        {
            emit sendStkDataList(StockDataList());
        }

        sleep(1);
    }


}

void QSinaStkResultMergeThread::slotRevResList(const StockDataList &mid)
{
    //qDebug()<<"recv Thread:"<<QThread::currentThreadId();
    mListMutex.lock();
    foreach (StockData data, mid) {
        if(data.cur < 0.1) continue;
        data.zjlx = mZjlxMaplist[data.code];
        mMidStkDataMapList[data.code] = data;
    }
    mListMutex.unlock();
}

void QSinaStkResultMergeThread::setStkList(const QStringList &list)
{
    mStkCodesList = list;
    mCodeChangeFlag = true;
}

void QSinaStkResultMergeThread::updateStkInfoList(const QList<QStringList>& pStkSectionList)
{

}

void QSinaStkResultMergeThread::setMktType(MKT_TYPE type)
{
    mMktType = type;
    mCurPage = 1;
    //updateStkCodes(mMktType);
}

void QSinaStkResultMergeThread::updateStkCodes(MKT_TYPE type)
{
    mSecCodesLock.lock();
    mSecCodesList.clear();
    if(type == MKT_ALL)
    {
        QStringList codes;
        foreach (QString code, mStkCodesList) {
            codes.append(code);
            if(codes.length() == 100)
            {
                mSecCodesList.append(codes);
                codes.clear();
            }
        }
        if(codes.length() > 0) mSecCodesList.append(codes);
    } else if(type == MKT_SH)
    {
        QStringList codes;
        foreach (QString code, mStkCodesList) {
            if(code.contains("sh"))
            {
                codes.append(code);
            } else
            {
                continue;
            }
            if(codes.length() == 100)
            {
                mSecCodesList.append(codes);
                codes.clear();
            }
        }
        if(codes.length() > 0) mSecCodesList.append(codes);
    } else if(type == MKT_SZ)
    {
        QStringList codes;
        foreach (QString code, mStkCodesList) {
            if(code.contains("sz"))
            {
                codes.append(code);
            } else
            {
                continue;
            }
            if(codes.length() == 100)
            {
                mSecCodesList.append(codes);
                codes.clear();
            }
        }
        if(codes.length() > 0) mSecCodesList.append(codes);
    } else if(type == MKT_ZXB)
    {
        QStringList codes;
        foreach (QString code, mStkCodesList) {
            if(code.startsWith("sz002"))
            {
                codes.append(code);
            } else
            {
                continue;
            }
            if(codes.length() == 100)
            {
                mSecCodesList.append(codes);
                codes.clear();
            }
        }
        if(codes.length() > 0) mSecCodesList.append(codes);
    } else if(type == MKT_CYB)
    {
        QStringList codes;
        foreach (QString code, mStkCodesList) {
            if(code.startsWith("sz30"))
            {
                codes.append(code);
            } else
            {
                continue;
            }
            if(codes.length() == 100)
            {
                mSecCodesList.append(codes);
                codes.clear();
            }
        }
        if(codes.length() > 0)mSecCodesList.append(codes);
    }
    mSecCodesLock.unlock();
//    updateStkInfoList(sec_codes_list);
}

void QSinaStkResultMergeThread::stopUpdate()
{

}

void QSinaStkResultMergeThread::SortResultList(StockDataList &result, const StockDataList &mid)
{

//    if(wklist.length())
//    {
//        //开始排序
//        if(mOptType == STK_DISPLAY_SORT_TYPE_CHGPER)
//        {
//            if(mSortRule == -1)
//            {
//                qSort(wklist.begin(), wklist.end(), StockData::sortByPerDesc);
//            } else
//            {
//                qSort(wklist.begin(), wklist.end(), StockData::sortByPerAsc);
//            }
//        }

//        else if(mOptType == STK_DISPLAY_SORT_TYPE_CJE)
//        {
//            if(mSortRule == -1)
//            {
//                qSort(wklist.begin(), wklist.end(), StockData::sortByMonDesc);
//            } else
//            {
//                qSort(wklist.begin(), wklist.end(), StockData::sortByMonAsc);
//            }
//        }

//        else if(mOptType == STK_DISPLAY_SORT_TYPE_PRICE)
//        {
//            if(mSortRule == -1)
//            {
//                qSort(wklist.begin(), wklist.end(), StockData::sortByCurDesc);
//            } else
//            {
//                qSort(wklist.begin(), wklist.end(), StockData::sortByCurAsc);
//            }
//        }
//        emit sendStkDataList(wklist);
//    }

//    reply->deleteLater();
//    QThread::sleep(1);
//}
}

StockDataList QSinaStkResultMergeThread::RealtimeInfo(const QStringList& codes)
{

}

void QSinaStkResultMergeThread::setSelfCodesList(const QStringList &list)
{
    mSelfCodesList = list;
    //qDebug()<<"selfcodes:"<<mSelfCodesList;
}

MktType QSinaStkResultMergeThread::getMktType()
{
    return mMktType;
}

void QSinaStkResultMergeThread::slotRevZjlxData(const QList<zjlxData> &zjlist)
{
    foreach (zjlxData data, zjlist) {
        mZjlxMaplist[data.code] = data.zjlx;
    }
}

void QSinaStkResultMergeThread::displayFirst()
{
    mCurPage = 1;
}

void QSinaStkResultMergeThread::displayLast()
{
    mCurPage = mTotalPage;
}

void QSinaStkResultMergeThread::displayPrevious()
{
    mCurPage--;
    if(mCurPage <= 0) mCurPage = 1;
}

void QSinaStkResultMergeThread::displayNext()
{
    mCurPage++;
    if(mCurPage > mTotalPage) mCurPage = mTotalPage;
}

void QSinaStkResultMergeThread::setCurPage(int page)
{
    mCurPage = page;
    if(mCurPage >= mTotalPage)
    {
        mCurPage = mTotalPage;
    }
    if(mCurPage <= 0)
        mCurPage = 1;
}

