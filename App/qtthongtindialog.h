#ifndef QTTHONGTINDIALOG_H
#define QTTHONGTINDIALOG_H

#include <QDialog>
#include <QNetworkAccessManager>
#include <QNetworkReply>

    namespace Ui {
    class QTThongTinDialog;
}

class QTThongTinDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QTThongTinDialog(const QString &token, const QString &tenDangNhap, QWidget *parent = nullptr);
    ~QTThongTinDialog();
    void setQuanTriInfo(const QString &hoVaTen, const QString &soDienThoai, const QString &gmail);

private slots:
    void on_pushButtonXacNhan_clicked();
    void on_pushButtonThoat_clicked();
    void handleUpdateQuanTriReply(QNetworkReply *reply);

private:
    Ui::QTThongTinDialog *ui;
    QNetworkAccessManager *networkManager;
    QString token;
    QString tenDangNhap;
};

#endif // QTTHONGTINDIALOG_H
