#include "quantriwindow.h"
#include "ui_quantriwindow.h"
#include "dangnhapwindow.h"
#include "doimatkhaudialog.h"
#include "qtthongtindialog.h" // Thêm include
#include "qtthemcongtydialog.h" // Thêm include
#include "qtxoacongtydialog.h" // Thêm include
#include "api.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>
#include <QTableWidgetItem>
#include <QDebug>
#include <QFont>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>

QuanTriWindow::QuanTriWindow(const QString &token, const QString &tenDangNhap, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::QuanTriWindow)
    , networkManager(new QNetworkAccessManager(this))
    , token(token)
    , tenDangNhap(tenDangNhap)
    , dangNhapWindow(nullptr)
{
    ui->setupUi(this);
    setWindowTitle("Quản trị");
    setAttribute(Qt::WA_QuitOnClose, false);

    // Kết nối các nút
    connect(ui->pushButtonDangXuat, &QPushButton::clicked, this, &QuanTriWindow::on_pushButtonDangXuat_clicked);
    connect(ui->pushButtonThongTinQuanTri, &QPushButton::clicked, this, &QuanTriWindow::on_pushButtonThongTinQuanTri_clicked);

    // Lấy thông tin quản trị viên ngay khi khởi tạo
    QNetworkRequest quanTriInfoRequest(QUrl(API+"/quantri-info/" + tenDangNhap));
    quanTriInfoRequest.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    QNetworkReply *quanTriInfoReply = networkManager->get(quanTriInfoRequest);
    connect(quanTriInfoReply, &QNetworkReply::finished, this, [=]() {
        handleQuanTriInfoReply(quanTriInfoReply);
        quanTriInfoReply->deleteLater();
    });

    // Lấy danh sách quản lý
    QNetworkRequest quanLyRequest(QUrl(API+"/quanly/"));
    quanLyRequest.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    QNetworkReply *quanLyReply = networkManager->get(quanLyRequest);
    connect(quanLyReply, &QNetworkReply::finished, this, [=]() {
        handleQuanLyReply(quanLyReply);
        quanLyReply->deleteLater();
    });
}

QuanTriWindow::~QuanTriWindow()
{
    delete ui;
    delete dangNhapWindow;
}

void QuanTriWindow::on_pushButtonDangXuat_clicked()
{
    isLogoutProcessed = true;
    if (!isLogoutProcessed) {
        qDebug() << "Logout already processed, ignoring";
        return; // Ngăn xử lý nếu đã đăng xuất
    }

    qDebug() << "Processing logout for QuanTriWindow";
    isLogoutProcessed = false;

    // Vô hiệu hóa nút và ngắt kết nối tín hiệu
    ui->pushButtonDangXuat->setEnabled(isLogoutProcessed);
    disconnect(ui->pushButtonDangXuat, &QPushButton::clicked, this, &QuanTriWindow::on_pushButtonDangXuat_clicked);

    dangNhapWindow = new DangNhapWindow(nullptr);
    dangNhapWindow->show();
    this->close();
}

void QuanTriWindow::on_pushButtonThemCongTy_clicked()
{
    QTThemCongTyDialog *dialog = new QTThemCongTyDialog(token, this);
    connect(dialog, &QDialog::accepted, this, [=]() {
        // Thêm độ trễ 500ms trước khi đóng và mở lại QuanTriWindow
        QTimer::singleShot(500, this, [=]() {
            // Đóng QuanTriWindow hiện tại và mở QuanTriWindow mới
            QuanTriWindow *newWindow = new QuanTriWindow(token, tenDangNhap, nullptr);
            newWindow->show();
            this->close();
        });
    });
    dialog->exec();
    delete dialog;

    disconnect(ui->pushButtonThemCongTy, &QPushButton::clicked, this, &QuanTriWindow::on_pushButtonThemCongTy_clicked);
    connect(ui->pushButtonThemCongTy, &QPushButton::clicked, this, &QuanTriWindow::on_pushButtonThemCongTy_clicked);
}

void QuanTriWindow::on_pushButtonXoaCongTy_clicked()
{
    qDebug() << "Opening QTXoaCongTyDialog";
    QNetworkRequest quanLyRequest(QUrl(API+"/quanly/"));
    quanLyRequest.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    QNetworkReply *quanLyReply = networkManager->get(quanLyRequest);
    connect(quanLyReply, &QNetworkReply::finished, this, [=]() {
        if (quanLyReply->error() != QNetworkReply::NoError) {
            QMessageBox::critical(this, "Lỗi", "Không thể lấy danh sách công ty: " + quanLyReply->errorString());
            qDebug() << "Failed to fetch companies:" << quanLyReply->errorString();
            quanLyReply->deleteLater();
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(quanLyReply->readAll());
        QJsonArray array = doc.array();

        qDebug() << "API /quanly/ response:" << QJsonDocument(array).toJson(QJsonDocument::Indented);

        if (array.isEmpty()) {
            QMessageBox::warning(this, "Cảnh báo", "Không có công ty nào để xóa.");
            qDebug() << "No companies available";
            quanLyReply->deleteLater();
            return;
        }

        qDebug() << "Loaded" << array.size() << "companies for deletion";
        QTXoaCongTyDialog *dialog = new QTXoaCongTyDialog(token, this);
        dialog->loadCompanies(array);
        connect(dialog, &QTXoaCongTyDialog::companiesDeleted, this, [=]() {
            qDebug() << "Companies deleted, refreshing table";
            QNetworkRequest refreshRequest(QUrl(API+"/quanly/"));
            refreshRequest.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
            QNetworkReply *refreshReply = networkManager->get(refreshRequest);
            connect(refreshReply, &QNetworkReply::finished, this, [=]() {
                handleQuanLyReply(refreshReply);
                QMessageBox::information(this, "Thành công", "Đã xóa các công ty được chọn.");
                refreshReply->deleteLater();
            });
        });
        dialog->exec();
        delete dialog;
        quanLyReply->deleteLater();
    });

    disconnect(ui->pushButtonXoaCongTy, &QPushButton::clicked, this, &QuanTriWindow::on_pushButtonXoaCongTy_clicked);
    connect(ui->pushButtonXoaCongTy, &QPushButton::clicked, this, &QuanTriWindow::on_pushButtonXoaCongTy_clicked);
}

void QuanTriWindow::on_pushButtonDoiMatKhau_clicked()
{
    DoiMatKhauDialog *dialog = new DoiMatKhauDialog(this);
    dialog->setTokenAndTenDangNhap(token, tenDangNhap);
    dialog->exec();
    delete dialog;
    disconnect(ui->pushButtonDoiMatKhau, &QPushButton::clicked, this, &QuanTriWindow::on_pushButtonDoiMatKhau_clicked);
    connect(ui->pushButtonDoiMatKhau, &QPushButton::clicked, this, &QuanTriWindow::on_pushButtonDoiMatKhau_clicked);

}

void QuanTriWindow::on_pushButtonThongTinQuanTri_clicked()
{
    // Gửi yêu cầu lấy thông tin quản trị viên
    QNetworkRequest quanTriInfoRequest(QUrl(API+"/quantri-info/" + tenDangNhap));
    quanTriInfoRequest.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    QNetworkReply *quanTriInfoReply = networkManager->get(quanTriInfoRequest);
    connect(quanTriInfoReply, &QNetworkReply::finished, this, [=]() {
        if (quanTriInfoReply->error() != QNetworkReply::NoError) {
            QMessageBox::critical(this, "Lỗi", "Không thể lấy thông tin quản trị: " + quanTriInfoReply->errorString());
            quanTriInfoReply->deleteLater();
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(quanTriInfoReply->readAll());
        QJsonObject obj = doc.object();

        QString hoVaTen = obj["HoVaTen"].toString();
        QString soDienThoai = obj["SoDienThoai"].toString();
        QString gmail = obj["Gmail"].toString();

        QTThongTinDialog *dialog = new QTThongTinDialog(token, tenDangNhap, this);
        dialog->setQuanTriInfo(hoVaTen, soDienThoai, gmail);
        connect(dialog, &QDialog::accepted, this, [=]() {
            // Làm mới thông tin quản trị
            QNetworkRequest refreshRequest(QUrl(API+"/quantri-info/" + tenDangNhap));
            refreshRequest.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
            QNetworkReply *refreshReply = networkManager->get(refreshRequest);
            connect(refreshReply, &QNetworkReply::finished, this, [=]() {
                handleQuanTriInfoReply(refreshReply);
                refreshReply->deleteLater();
            });
        });
        dialog->exec();
        delete dialog;
        quanTriInfoReply->deleteLater();
    });

    disconnect(ui->pushButtonThongTinQuanTri, &QPushButton::clicked, this, &QuanTriWindow::on_pushButtonThongTinQuanTri_clicked);
    connect(ui->pushButtonThongTinQuanTri, &QPushButton::clicked, this, &QuanTriWindow::on_pushButtonThongTinQuanTri_clicked);
}

void QuanTriWindow::handleQuanLyReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::critical(this, "Lỗi", "Không thể lấy danh sách quản lý: " + reply->errorString());
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonArray array = doc.array();

    // Thiết lập bảng quản lý (6 cột: Tên công ty, Họ và tên, Số điện thoại, Gmail, Số khách hàng, Chi tiết)
    ui->tableWidgetThongTinQuanLy->setColumnCount(6);
    ui->tableWidgetThongTinQuanLy->setHorizontalHeaderLabels({
        "Tên công ty",
        "Họ và tên",
        "Số điện thoại",
        "Gmail",
        "Số khách hàng",
        "Chi tiết"
    });

    // In đậm tiêu đề các cột
    QFont headerFont = ui->tableWidgetThongTinQuanLy->horizontalHeader()->font();
    headerFont.setBold(true);
    ui->tableWidgetThongTinQuanLy->horizontalHeader()->setFont(headerFont);

    ui->tableWidgetThongTinQuanLy->setRowCount(array.size());
    ui->tableWidgetThongTinQuanLy->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    for (int i = 0; i < array.size(); ++i) {
        QJsonObject obj = array[i].toObject();

        // Tạo các item và căn giữa
        QTableWidgetItem *congTyItem = new QTableWidgetItem(obj["CongTy"].toString());
        congTyItem->setTextAlignment(Qt::AlignCenter);
        congTyItem->setData(Qt::UserRole, obj["TenDangNhap"].toString());
        ui->tableWidgetThongTinQuanLy->setItem(i, 0, congTyItem);

        QTableWidgetItem *hoVaTenItem = new QTableWidgetItem(obj["HoVaTen"].toString());
        hoVaTenItem->setTextAlignment(Qt::AlignCenter);
        ui->tableWidgetThongTinQuanLy->setItem(i, 1, hoVaTenItem);

        QTableWidgetItem *soDienThoaiItem = new QTableWidgetItem(obj["SoDienThoai"].toString());
        soDienThoaiItem->setTextAlignment(Qt::AlignCenter);
        ui->tableWidgetThongTinQuanLy->setItem(i, 2, soDienThoaiItem);

        QTableWidgetItem *gmailItem = new QTableWidgetItem(obj["Gmail"].toString());
        gmailItem->setTextAlignment(Qt::AlignCenter);
        ui->tableWidgetThongTinQuanLy->setItem(i, 3, gmailItem);

        QTableWidgetItem *customerCountItem = new QTableWidgetItem(QString::number(obj["CustomerCount"].toInt()));
        customerCountItem->setTextAlignment(Qt::AlignCenter);
        ui->tableWidgetThongTinQuanLy->setItem(i, 4, customerCountItem);

        // Tạo nút Chi tiết với chữ gạch chân và nghiêng
        QPushButton *detailButton = new QPushButton("Chi tiết");
        QFont font = detailButton->font();
        font.setItalic(true);
        font.setUnderline(true);
        detailButton->setFont(font);
        detailButton->setStyleSheet("border: none;");
        ui->tableWidgetThongTinQuanLy->setCellWidget(i, 5, detailButton);

        // Kết nối tín hiệu clicked của nút với slot, truyền số hàng
        connect(detailButton, &QPushButton::clicked, this, [=]() {
            onDetailButtonClicked(i);
        });
    }
}

void QuanTriWindow::handleQuanTriInfoReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::critical(this, "Lỗi", "Không thể lấy thông tin quản trị: " + reply->errorString());
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject obj = doc.object();

    QString hoVaTen = obj["HoVaTen"].toString();
    if (!hoVaTen.isEmpty()) {
        ui->pushButtonThongTinQuanTri->setText(hoVaTen);
    } else {
        ui->pushButtonThongTinQuanTri->setText("Quản trị viên");
    }
}

void QuanTriWindow::onDetailButtonClicked(int row)
{
    // Lấy TenDangNhap từ thuộc tính userData của ô cột 0
    QTableWidgetItem *item = ui->tableWidgetThongTinQuanLy->item(row, 0);
    if (!item) {
        QMessageBox::critical(this, "Lỗi", "Không thể lấy thông tin tài khoản: Dữ liệu không hợp lệ");
        return;
    }
    QString tenDangNhap = item->data(Qt::UserRole).toString();

    // Gửi yêu cầu lấy thông tin tài khoản quản lý
    QNetworkRequest taikhoanRequest(QUrl(API+"/taikhoan/by-ten-dang-nhap/" + tenDangNhap));
    taikhoanRequest.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    QNetworkReply *taikhoanReply = networkManager->get(taikhoanRequest);
    connect(taikhoanReply, &QNetworkReply::finished, this, [=]() {
        handleTaiKhoanReply(taikhoanReply);
        taikhoanReply->deleteLater();
    });
}

void QuanTriWindow::handleTaiKhoanReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        QString errorMsg = "Không thể lấy thông tin tài khoản: " + reply->errorString();
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj.contains("detail")) {
                errorMsg += "\nChi tiết: " + obj["detail"].toString();
            }
        }
        QMessageBox::critical(this, "Lỗi", errorMsg);
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject obj = doc.object();

    if (obj.isEmpty() || obj["VaiTro"].toString() != "QuanLy") {
        QMessageBox::critical(this, "Lỗi", "Không tìm thấy thông tin tài khoản quản lý hoặc vai trò không hợp lệ");
        return;
    }

    showQuanLyDetails(obj);
}

void QuanTriWindow::showQuanLyDetails(const QJsonObject &taikhoan)
{
    // Tạo dialog hiển thị thông tin
    QDialog *detailDialog = new QDialog(this);
    detailDialog->setWindowTitle("Thông tin tài khoản quản lý");

    // Thu nhỏ chiều dài dialog khoảng 25% (từ 400px xuống 300px)
    detailDialog->setFixedSize(300, 200);

    QVBoxLayout *layout = new QVBoxLayout(detailDialog);
    // Căn giữa các thành phần trong layout
    layout->setAlignment(Qt::AlignCenter);

    QLabel *tenDangNhapLabel = new QLabel("Tên đăng nhập: " + taikhoan["TenDangNhap"].toString());
    tenDangNhapLabel->setAlignment(Qt::AlignCenter);

    QLabel *matKhauLabel = new QLabel("Mật khẩu: " + taikhoan["MatKhau"].toString());
    matKhauLabel->setAlignment(Qt::AlignCenter);

    QLabel *ngayTaoLabel = new QLabel("Ngày tạo: " + taikhoan["NgayTao"].toString());
    ngayTaoLabel->setAlignment(Qt::AlignCenter);

    QPushButton *closeButton = new QPushButton("Đóng");
    closeButton->setFixedWidth(100);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);
    buttonLayout->addStretch();

    connect(closeButton, &QPushButton::clicked, detailDialog, &QDialog::close);

    layout->addWidget(tenDangNhapLabel);
    layout->addWidget(matKhauLabel);
    layout->addWidget(ngayTaoLabel);
    layout->addLayout(buttonLayout);
    detailDialog->setLayout(layout);

    detailDialog->exec();
}
