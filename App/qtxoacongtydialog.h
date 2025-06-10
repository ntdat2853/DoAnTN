#ifndef QTXOACONGTYDIALOG_H
#define QTXOACONGTYDIALOG_H

#include <QDialog>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QCheckBox>

namespace Ui {
class QTXoaCongTyDialog;
}

class QTXoaCongTyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QTXoaCongTyDialog(const QString &token, QWidget *parent = nullptr);
    ~QTXoaCongTyDialog();
    void loadCompanies(const QJsonArray &companies);

signals:
    void companiesDeleted(); // Moved to signals section

private slots:
    void on_pushButtonXacNhan_clicked();
    void on_pushButtonThoat_clicked();
    void handleDeleteCongTyReply(QNetworkReply *reply);

private:
    Ui::QTXoaCongTyDialog *ui;
    QNetworkAccessManager *networkManager;
    QString token;
    QList<QCheckBox*> checkBoxes;
};

#endif // QTXOACONGTYDIALOG_H
