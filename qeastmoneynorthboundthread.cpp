#include "qeastmoneynorthboundthread.h"
#include "qhttpget.h"
#include "QRegExp"
#include <QDebug>

QEastmoneyNorthBoundThread::QEastmoneyNorthBoundThread(QObject *parent):QThread(parent)
{

}

double QEastmoneyNorthBoundThread::changeRMBString(const QString &src)
{
    QRegExp exp("([0-9\.\-]{1,})");
    int index = exp.indexIn(src, 0);
    if(index != -1)
    {
        double value = exp.cap().toDouble();
        if(src.contains("亿元"))
        {
            return value * 100000000;
        }
        return value * 10000;
    }

    return 0.0;
}

void QEastmoneyNorthBoundThread::run()
{
    while (true) {
        QString url = tr("http://nufm.dfcfw.com/EM_Finance2014NumericApplication/JS.aspx?type=CT&cmd=P.(x),(x),(x)|0000011|3990012|3990012,BK07071,BK08041&sty=SHSTD|SZSTD|FCSHSTR&st=z&sr=&p=&ps=&cb=&js=(x)&token=1942f5da9b46b069953c873404aad4b5");
        QString result = QString::fromUtf8(QHttpGet().getContent(url));
        //开始解析
        //"5.12亿元,99.88亿元,105亿元,0,0.00万元,2500亿元,12.93亿元,117.07亿元,130亿元,0,0.00万元,0万元","2.92亿元,102.08亿元,105亿元,0,0.00万元,0万元,9.19亿元,120.81亿元,130亿元,0,0.00万元,0万元","399001,2,深证成指,0.23%,310|50|140|37","BK0707,1,沪股通,0.57%,349|59|166|31","BK0804,1,深股通,0.60%,608|109|241|84"
        QStringList splitstr = result.split(QRegExp("\"|\","), QString::SkipEmptyParts);
        QString hgt_pure_in = "0.0";
        QString sgt_pure_in = "0.0";
        if(splitstr.length() != 5) return;
        QString sh_north = splitstr[0];
        hgt_pure_in = sh_north.split(",").at(6);
        QString sz_north = splitstr[1];
        sgt_pure_in = sz_north.split(",").at(6);
        double pure_in[2];
        pure_in[0] = changeRMBString(hgt_pure_in) / 100000000;
        pure_in[1] = changeRMBString(sgt_pure_in) / 100000000;

        StockDataList wklist;
        for(int i=3; i<splitstr.length(); i++)
        {
            QStringList hgt = splitstr[i].split(",");
            StockData data;
            data.code = hgt[0];
            data.name = hgt[2];
            data.per = hgt[3].left(hgt[3].length() -1).toDouble();
            data.cur = pure_in[i-3];
            data.chg = data.cur / 130;
            wklist.append(data);
        }

        emit signalUpdateNorthBoundList(wklist);

        QThread::sleep(1*60);
    }


}
