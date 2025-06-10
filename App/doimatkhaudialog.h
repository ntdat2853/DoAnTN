#ifndef DOIMATKHAUDIALOG_H
#define DOIMATKHAUDIALOG_H

#include <QDialog>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace Ui {
class DoiMatKhauDialog;
}

class DoiMatKhauDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DoiMatKhauDialog(QWidget *parent = nullptr);
    ~DoiMatKhauDialog();
    void setTokenAndTenDangNhap(const QString &token, const QString &tenDangNhap);

private slots:
    void on_pushButtonXacNhan_clicked();
    void on_pushButtonThoat_clicked();
    void handleChangePasswordReply(QNetworkReply *reply);

private:
    Ui::DoiMatKhauDialog *ui;
    QNetworkAccessManager *networkManager;
    QString token;
    QString tenDangNhap;
};

#endif // DOIMATKHAUDIALOG_H
