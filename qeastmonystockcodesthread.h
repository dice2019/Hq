#ifndef QEASTMONYSTOCKCODESTHREAD_H
#define QEASTMONYSTOCKCODESTHREAD_H

#include <QThread>

class QEastMonyStockCodesThread : public QThread
{
    Q_OBJECT
public:
    explicit QEastMonyStockCodesThread(QObject *parent = 0);
    ~QEastMonyStockCodesThread();

protected:
    void run();

signals:
    void signalSendCodesList(const QStringList& list);

public slots:
};

#endif // QEASTMONYSTOCKCODESTHREAD_H
