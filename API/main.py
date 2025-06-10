import logging
from fastapi import FastAPI, Depends, HTTPException, Request
from fastapi.responses import JSONResponse
from sqlalchemy.orm import Session
from typing import List, Optional
from database import get_db
from models import TaiKhoan, QuanLy, KhachHang, QuanTri, GiaoDich, ThanhToan, TriggerLog
from schemas import (
    TaiKhoan as TaiKhoanSchema, TaiKhoanCreate, QuanLy as QuanLySchema, QuanLyCreate, QuanLyUpdate,
    KhachHang as KhachHangSchema, KhachHangCreate, KhachHangUpdate, QuanTri as QuanTriSchema, QuanTriCreate,
    QuanTriUpdate, GiaoDich as GiaoDichSchema, GiaoDichCreate, ThanhToan as ThanhToanSchema,
    TriggerLog as TriggerLogSchema, LoginRequest, Token, GiaMuCreate, GiaMuUpdate, PasswordUpdateRequest,
    GiaMu as GiaMuSchema, PasswordResetRequest
)
from schemas import GiaoDichMuTapCreate, GiaoDichMuNuocCreate, GiaoDichTSCDRCCreate, GiaoDich
import crud
from datetime import date
from pydantic import BaseModel
from typing import List

# Setup logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

app = FastAPI(title="QuanLyMuCaoSu API", description="API for managing water and impurity transactions")

class DeleteCongTyRequest(BaseModel):
    cong_ty_list: List[str]

# Middleware để ghi lại lỗi
@app.middleware("http")
async def log_exceptions(request: Request, call_next):
    logger.info(f"Received request: {request.method} {request.url}")
    try:
        response = await call_next(request)
        return response
    except Exception as e:
        logger.error(f"Unhandled exception: {str(e)}", exc_info=True)
        return JSONResponse(
            status_code=500,
            content={"detail": f"Internal Server Error: {str(e)}"}
        )

# Login Endpoint
@app.post("/login", response_model=Token, summary="User login to obtain JWT token")
def login(login_request: LoginRequest, db: Session = Depends(get_db)):
    """Authenticate user and return a JWT access token."""
    logger.info(f"Login attempt for TenDangNhap: {login_request.TenDangNhap}")
    try:
        token = crud.login(db, login_request)
        return token
    except HTTPException as e:
        raise e
    except Exception as e:
        logger.error(f"Unexpected error during login: {str(e)}")
        raise HTTPException(status_code=500, detail="Internal Server Error")

# TaiKhoan Endpoints
@app.post("/taikhoan/", response_model=TaiKhoanSchema, summary="Create a new TaiKhoan")
def create_taikhoan(taikhoan: TaiKhoanCreate, db: Session = Depends(get_db)):
    """Create a new user account with specified role (KhachHang, QuanLy, QuanTri)."""
    logger.info(f"Received request to create TaiKhoan: {taikhoan.dict()}")
    return crud.create_taikhoan(db, taikhoan)

@app.get("/taikhoan/{taikhoan_id}", response_model=TaiKhoanSchema, summary="Get TaiKhoan by ID")
def read_taikhoan(taikhoan_id: int, db: Session = Depends(get_db)):
    """Retrieve a TaiKhoan by its ID."""
    db_taikhoan = crud.get_taikhoan(db, taikhoan_id)
    if db_taikhoan is None:
        raise HTTPException(status_code=404, detail="TaiKhoan not found")
    return db_taikhoan

@app.get("/taikhoan/by-ten-dang-nhap/{ten_dang_nhap}", response_model=TaiKhoanSchema, summary="Get TaiKhoan by TenDangNhap")
def read_taikhoan_by_ten_dang_nhap(ten_dang_nhap: str, db: Session = Depends(get_db)):
    """Retrieve a TaiKhoan by its TenDangNhap."""
    db_taikhoan = crud.get_taikhoan_by_ten_dang_nhap(db, ten_dang_nhap)
    if db_taikhoan is None:
        raise HTTPException(status_code=404, detail="TaiKhoan not found")
    return db_taikhoan

@app.get("/taikhoan/", response_model=List[TaiKhoanSchema], summary="Get list of TaiKhoan")
def read_taikhoans(skip: int = 0, limit: int = 100, db: Session = Depends(get_db)):
    """Retrieve a paginated list of TaiKhoan."""
    logger.info(f"Fetching TaiKhoan with skip={skip}, limit={limit}")
    try:
        taikhoans = crud.get_taikhoans(db, skip, limit)
        logger.info(f"Returning {len(taikhoans)} TaiKhoan records")
        return taikhoans
    except Exception as e:
        logger.error(f"Error fetching TaiKhoan: {str(e)}")
        raise HTTPException(status_code=500, detail="Internal Server Error")

@app.put("/taikhoan/{taikhoan_id}", response_model=TaiKhoanSchema, summary="Update TaiKhoan")
def update_taikhoan(taikhoan_id: int, taikhoan: TaiKhoanCreate, db: Session = Depends(get_db)):
    """Update an existing TaiKhoan by its ID."""
    return crud.update_taikhoan(db, taikhoan_id, taikhoan)

@app.put("/taikhoan/{ten_dang_nhap}/change-password", summary="Change password for a TaiKhoan")
async def change_password(ten_dang_nhap: str, password_update: PasswordUpdateRequest, db: Session = Depends(get_db)):
    """Change the password for a TaiKhoan by TenDangNhap."""
    logger.info(f"Changing password for TenDangNhap: {ten_dang_nhap}")
    return crud.update_password(db, ten_dang_nhap, password_update)

@app.put("/taikhoan/reset-password/{ten_dang_nhap}", summary="Reset password for a TaiKhoan")
async def reset_password(ten_dang_nhap: str, password_reset: PasswordResetRequest, db: Session = Depends(get_db)):
    """Reset the password for a TaiKhoan by TenDangNhap without requiring current password."""
    logger.info(f"Resetting password for TenDangNhap: {ten_dang_nhap}")
    db_taikhoan = db.query(TaiKhoan).filter(TaiKhoan.TenDangNhap == ten_dang_nhap).first()
    if not db_taikhoan:
        logger.warning(f"TaiKhoan with TenDangNhap {ten_dang_nhap} not found")
        raise HTTPException(status_code=404, detail="TaiKhoan not found")

    db_taikhoan.MatKhau = password_reset.new_password
    try:
        db.commit()
        db.refresh(db_taikhoan)
        logger.info(f"Password reset for TenDangNhap: {ten_dang_nhap}")
        return {"detail": "Mật khẩu đã được đặt lại"}
    except Exception as e:
        db.rollback()
        logger.error(f"Unexpected error: {str(e)}")
        raise HTTPException(status_code=500, detail="Internal Server Error")

# QuanTri Endpoints
@app.post("/quantri/", response_model=QuanTriSchema, summary="Create a new QuanTri")
def create_quantri(quantri: QuanTriCreate, db: Session = Depends(get_db)):
    """Create a new QuanTri account."""
    return crud.create_quantri(db, quantri)

@app.get("/quantri/{quantri_id}", response_model=QuanTriSchema, summary="Get QuanTri by ID")
def read_quantri(quantri_id: int, db: Session = Depends(get_db)):
    """Retrieve a QuanTri by its ID."""
    db_quantri = crud.get_quantri(db, quantri_id)
    if db_quantri is None:
        raise HTTPException(status_code=404, detail="QuanTri not found")
    return db_quantri

@app.get("/quantri-info/{ten_dang_nhap}", summary="Get QuanTri info by TenDangNhap")
def get_quantri_info(ten_dang_nhap: str, db: Session = Depends(get_db)):
    """Retrieve QuanTri information (HoVaTen, SoDienThoai, Gmail) by TenDangNhap."""
    return crud.get_quantri_info(db, ten_dang_nhap)

@app.post("/quantri/update-info/", summary="Update QuanTri info")
def update_quantri_info(quantri_update: QuanTriUpdate, db: Session = Depends(get_db)):
    """Update QuanTri information using stored procedure."""
    crud.update_quantri_info(db, quantri_update)
    return {"detail": f"Updated QuanTri info for TenDangNhap: {quantri_update.ten_dang_nhap}"}

# QuanLy Endpoints
@app.post("/quanly/", response_model=QuanLySchema, summary="Create a new QuanLy")
def create_quanly(quanly: QuanLyCreate, db: Session = Depends(get_db)):
    """Create a new QuanLy account."""
    return crud.create_quanly(db, quanly)

@app.get("/quanly/{quanly_id}", response_model=QuanLySchema, summary="Get QuanLy by ID")
def read_quanly(quanly_id: int, db: Session = Depends(get_db)):
    """Retrieve a QuanLy by its ID."""
    db_quanly = crud.get_quanly(db, quanly_id)
    if db_quanly is None:
        raise HTTPException(status_code=404, detail="QuanLy not found")
    return db_quanly

@app.get("/quanly-info/{ten_dang_nhap}", summary="Get QuanLy info by TenDangNhap")
def get_quanly_info(ten_dang_nhap: str, db: Session = Depends(get_db)):
    """Retrieve QuanLy information (HoVaTen, SoDienThoai, Gmail, CongTy, CustomerCount) by TenDangNhap."""
    return crud.get_quanly_info(db, ten_dang_nhap)

@app.post("/quanly/update-info/", summary="Update QuanLy info")
def update_quanly_info(quanly_update: QuanLyUpdate, db: Session = Depends(get_db)):
    """Update QuanLy information using stored procedure."""
    crud.update_quanly_info(db, quanly_update)
    return {"detail": f"Updated QuanLy info for TenDangNhap: {quanly_update.ten_dang_nhap}"}

@app.delete("/quanly/{taikhoan_id}", summary="Delete QuanLy by IDTaiKhoan")
def delete_quanly(taikhoan_id: int, db: Session = Depends(get_db)):
    """Delete a QuanLy and all associated customers by IDTaiKhoan."""
    return crud.delete_quanly_by_id_taikhoan(db, taikhoan_id)

@app.get("/quanly/", summary="Get list of QuanLy")
def read_quanly(skip: int = 0, limit: int = 100, db: Session = Depends(get_db)):
    """Retrieve a paginated list of QuanLy with CustomerCount."""
    logger.info(f"Fetching QuanLy with skip={skip}, limit={limit}")
    try:
        # Lấy danh sách quản lý
        quanly_list = crud.get_quanly(db, skip, limit)

        # Tạo danh sách kết quả với thêm trường CustomerCount
        result = []
        for quanly in quanly_list:
            # Đếm số khách hàng thuộc công ty của quản lý
            customer_count = db.query(KhachHang).filter(KhachHang.CongTy == quanly.CongTy).count()

            # Tạo dictionary với dữ liệu cần thiết
            quanly_data = {
                "IDQuanLy": quanly.IDQuanLy,
                "IDTaiKhoan": quanly.IDTaiKhoan,
                "HoVaTen": quanly.HoVaTen if quanly.HoVaTen else "",
                "SoDienThoai": quanly.SoDienThoai if quanly.SoDienThoai else "",
                "Gmail": quanly.Gmail if quanly.Gmail else "",
                "CongTy": quanly.CongTy if quanly.CongTy else "",
                "TenDangNhap": quanly.taikhoan.TenDangNhap if quanly.taikhoan else "",
                "CustomerCount": customer_count
            }
            result.append(quanly_data)

        logger.info(f"Returning {len(result)} QuanLy records")
        return result
    except Exception as e:
        logger.error(f"Error fetching QuanLy: {str(e)}")
        raise HTTPException(status_code=500, detail="Internal Server Error")

@app.post("/quanly/create-with-phone/", response_model=QuanLySchema, summary="Create a new QuanLy with phone as credentials")
def create_quanly_with_phone(quanly: QuanLyCreate, db: Session = Depends(get_db)):
    """Create a new QuanLy account using phone number as username and password."""
    logger.info(f"Received request to create QuanLy with phone: {quanly.SODienThoai}")
    if not quanly.SODienThoai:
        logger.warning("SODienThoai is None or empty")
        raise HTTPException(status_code=400, detail="Số điện thoại không được để trống")
    return crud.create_quanly_with_phone(db, quanly)

@app.delete("/quanly/delete-by-congty/", summary="Delete QuanLy by company list")
def delete_quanly_by_congty(request: DeleteCongTyRequest, db: Session = Depends(get_db)):
    """Delete QuanLy accounts and associated data by company list."""
    logger.info(f"Received request to delete companies: {request.cong_ty_list}")
    return crud.delete_quanly_by_congty(db, request.cong_ty_list)

# KhachHang Endpoints
@app.post("/khachhang/", response_model=KhachHangSchema, summary="Create a new KhachHang")
def create_khachhang(khachhang: KhachHangCreate, db: Session = Depends(get_db)):
    """Create a new KhachHang account."""
    return crud.create_khachhang(db, khachhang)

@app.get("/khachhang/{khachhang_id}", response_model=KhachHangSchema, summary="Get KhachHang by ID")
def read_khachhang(khachhang_id: int, db: Session = Depends(get_db)):
    """Retrieve a KhachHang by its ID."""
    db_khachhang = crud.get_khachhang(db, khachhang_id)
    if db_khachhang is None:
        raise HTTPException(status_code=404, detail="KhachHang not found")
    return db_khachhang

@app.get("/khachhang-info/{ten_dang_nhap}", summary="Get KhachHang info by TenDangNhap")
def get_khachhang_info(ten_dang_nhap: str, db: Session = Depends(get_db)):
    """Retrieve KhachHang information (HoVaTen, SoDienThoai, Gmail, CongTy, SoTaiKhoan, NganHang, RFID) by TenDangNhap."""
    return crud.get_khachhang_info(db, ten_dang_nhap)

@app.get("/khachhang/", response_model=List[KhachHangSchema], summary="Get list of KhachHang")
def read_khachhangs(cong_ty: Optional[str] = None, skip: int = 0, limit: int = 100, db: Session = Depends(get_db)):
    """Retrieve a paginated list of KhachHang, optionally filtered by CongTy."""
    logger.info(f"Fetching KhachHang with skip={skip}, limit={limit}, cong_ty={cong_ty}")
    try:
        query = db.query(KhachHang)
        if cong_ty:
            query = query.filter(KhachHang.CongTy == cong_ty)
        khachhangs = query.offset(skip).limit(limit).all()
        # Ánh xạ dữ liệu để bao gồm TenDangNhap
        result = []
        for khachhang in khachhangs:
            taikhoan = db.query(TaiKhoan).filter(TaiKhoan.IDTaiKhoan == khachhang.IDTaiKhoan).first()
            khachhang_dict = {
                "IDKhachHang": khachhang.IDKhachHang,
                "IDTaiKhoan": khachhang.IDTaiKhoan,
                "HoVaTen": khachhang.HoVaTen if khachhang.HoVaTen else "",
                "SoDienThoai": khachhang.SoDienThoai if khachhang.SoDienThoai else "",
                "Gmail": khachhang.Gmail if khachhang.Gmail else "",
                "CongTy": khachhang.CongTy if khachhang.CongTy else "",
                "SoTaiKhoan": khachhang.SoTaiKhoan if khachhang.SoTaiKhoan else "",
                "NganHang": khachhang.NganHang if khachhang.NganHang else "",
                "RFID": khachhang.RFID if khachhang.RFID else "",
                "ten_dang_nhap": taikhoan.TenDangNhap if taikhoan else ""
            }
            result.append(khachhang_dict)
        logger.info(f"Returning {len(result)} KhachHang records")
        return result
    except Exception as e:
        logger.error(f"Error fetching KhachHang: {str(e)}")
        raise HTTPException(status_code=500, detail=f"Internal Server Error: {str(e)}")

@app.post("/khachhang/update-info/", summary="Update KhachHang info")
def update_khachhang_info(khachhang_update: KhachHangUpdate, db: Session = Depends(get_db)):
    """Update KhachHang information using stored procedure."""
    crud.update_khachhang_info(db, khachhang_update)
    return {"detail": f"Updated KhachHang info for TenDangNhap: {khachhang_update.ten_dang_nhap}"}

@app.delete("/khachhang/{khachhang_id}", summary="Delete KhachHang by IDKhachHang")
async def delete_khach_hang(khachhang_id: int, db: Session = Depends(get_db)):
    """Delete a KhachHang by IDKhachHang, including related data in TaiKhoan, GiaoDich, and ThanhToan."""
    logger.info(f"Deleting KhachHang with IDKhachHang: {khachhang_id}")
    try:
        result = crud.delete_khach_hang_by_id_khachhang(db, khachhang_id)
        logger.info(f"Successfully deleted KhachHang with IDKhachHang: {khachhang_id}")
        return {"detail": f"Đã xóa khách hàng với IDKhachHang: {khachhang_id}"}
    except HTTPException as e:
        raise e
    except Exception as e:
        logger.error(f"Unexpected error while deleting KhachHang: {str(e)}")
        raise HTTPException(status_code=500, detail=f"Internal Server Error: {str(e)}")

@app.post("/quanly/delete-by-congty/", summary="Delete QuanLy by company list")  # Thêm POST
def delete_quanly_by_congty(request: DeleteCongTyRequest, db: Session = Depends(get_db)):
    """Delete QuanLy accounts and associated data by company list."""
    logger.info(f"Received request to delete companies: {request.cong_ty_list}")
    return crud.delete_quanly_by_congty(db, request.cong_ty_list)
# GiaMu Endpoints
@app.post("/giamu/", response_model=GiaMuSchema, summary="Create a new GiaMu")
def create_gia_mu(gia_mu: GiaMuCreate, db: Session = Depends(get_db)):
    """Create a new GiaMu record."""
    return crud.create_gia_mu(db, gia_mu)

@app.put("/giamu/", response_model=GiaMuSchema, summary="Update GiaMu")
def update_gia_mu(gia_mu_update: GiaMuUpdate, db: Session = Depends(get_db)):
    """Update GiaMuNuoc and GiaMuTap for a specific date and CongTy."""
    return crud.update_gia_mu(db, gia_mu_update)

@app.get("/giamu/latest/{cong_ty}", response_model=GiaMuSchema, summary="Get latest GiaMu by CongTy")
def get_latest_gia_mu(cong_ty: str, db: Session = Depends(get_db)):
    """Retrieve the latest GiaMu record for a specific CongTy."""
    if not cong_ty:
        raise HTTPException(status_code=400, detail="CongTy cannot be empty")
    try:
        return crud.get_latest_gia_mu(db, cong_ty)
    except HTTPException as e:
        raise e
    except Exception as e:
        logger.error(f"Unexpected error while fetching latest GiaMu: {str(e)}")
        raise HTTPException(status_code=500, detail=f"Unexpected error: {str(e)}")

@app.get("/giamu/{ngay_thiet_lap}/{cong_ty}", response_model=GiaMuSchema, summary="Get GiaMu by date and CongTy")
def get_gia_mu_by_date_and_company(ngay_thiet_lap: date, cong_ty: str, db: Session = Depends(get_db)):
    """Retrieve GiaMu for a specific date and CongTy."""
    return crud.get_gia_mu_by_date_and_company(db, ngay_thiet_lap, cong_ty)

# GiaoDich Endpoints
@app.post("/giaodich/", response_model=GiaoDichSchema, summary="Create a new GiaoDich")
def create_giaodich(giaodich: GiaoDichCreate, db: Session = Depends(get_db)):
    """Create a new transaction for a KhachHang."""
    return crud.create_giaodich(db, giaodich)

@app.get("/giaodich/{giaodich_id}", response_model=GiaoDichSchema, summary="Get GiaoDich by ID")
def read_giaodich(giaodich_id: int, db: Session = Depends(get_db)):
    """Retrieve a GiaoDich by its ID."""
    db_giaodich = crud.get_giaodich(db, giaodich_id)
    if db_giaodich is None:
        raise HTTPException(status_code=404, detail="GiaoDich not found")
    return {
        "IDGiaoDich": db_giaodich.IDGiaoDich,
        "IDKhachHang": db_giaodich.IDKhachHang,
        "NgayGiaoDich": db_giaodich.NgayGiaoDich,
        "ThoiGianGiaoDich": db_giaodich.ThoiGianGiaoDich.strftime("%H:%M:%S"),
        "CongTy": db_giaodich.CongTy,
        "KhoiLuongMuNuoc": float(db_giaodich.MuNuoc) if db_giaodich.MuNuoc is not None else 0.0,
        "DoMuNuoc": float(db_giaodich.TSC) if db_giaodich.TSC is not None else 0.0,
        "KhoiLuongMuTap": float(db_giaodich.MuTap) if db_giaodich.MuTap is not None else 0.0,
        "DoMuTap": float(db_giaodich.DRC) if db_giaodich.DRC is not None else 0.0,
        "MuNuoc": float(db_giaodich.MuNuoc) if db_giaodich.MuNuoc is not None else 0.0,
        "TSC": float(db_giaodich.TSC) if db_giaodich.TSC is not None else 0.0,
        "GiaMuNuoc": float(db_giaodich.GiaMuNuoc) if db_giaodich.GiaMuNuoc is not None else 0.0,
        "MuTap": float(db_giaodich.MuTap) if db_giaodich.MuTap is not None else 0.0,
        "DRC": float(db_giaodich.DRC) if db_giaodich.DRC is not None else 0.0,
        "GiaMuTap": float(db_giaodich.GiaMuTap) if db_giaodich.GiaMuTap is not None else 0.0,
        "TongTien": float(db_giaodich.TongTien) if db_giaodich.TongTien is not None else 0.0
    }

@app.get("/giaodich/khachhang/{khachhang_id}", response_model=List[GiaoDichSchema], summary="Get GiaoDich by KhachHang")
def read_giaodichs_by_khachhang(khachhang_id: int, db: Session = Depends(get_db)):
    """Retrieve all transactions for a KhachHang."""
    return crud.get_giaodichs_by_khachhang(db, khachhang_id)

@app.put("/giaodich/{giaodich_id}", response_model=GiaoDichSchema, summary="Update GiaoDich")
def update_giaodich(giaodich_id: int, giaodich: GiaoDichCreate, db: Session = Depends(get_db)):
    """Update an existing GiaoDich by its ID."""
    updated_giaodich = crud.update_giaodich(db, giaodich_id, giaodich)
    return {
        "IDGiaoDich": updated_giaodich.IDGiaoDich,
        "IDKhachHang": updated_giaodich.IDKhachHang,
        "NgayGiaoDich": updated_giaodich.NgayGiaoDich,
        "ThoiGianGiaoDich": updated_giaodich.ThoiGianGiaoDich.strftime("%H:%M:%S"),
        "CongTy": updated_giaodich.CongTy,
        "KhoiLuongMuNuoc": float(updated_giaodich.MuNuoc) if updated_giaodich.MuNuoc is not None else 0.0,
        "DoMuNuoc": float(updated_giaodich.TSC) if updated_giaodich.TSC is not None else 0.0,
        "KhoiLuongMuTap": float(updated_giaodich.MuTap) if updated_giaodich.MuTap is not None else 0.0,
        "DoMuTap": float(updated_giaodich.DRC) if updated_giaodich.DRC is not None else 0.0,
        "MuNuoc": float(updated_giaodich.MuNuoc) if updated_giaodich.MuNuoc is not None else 0.0,
        "TSC": float(updated_giaodich.TSC) if updated_giaodich.TSC is not None else 0.0,
        "GiaMuNuoc": float(updated_giaodich.GiaMuNuoc) if updated_giaodich.GiaMuNuoc is not None else 0.0,
        "MuTap": float(updated_giaodich.MuTap) if updated_giaodich.MuTap is not None else 0.0,
        "DRC": float(updated_giaodich.DRC) if updated_giaodich.DRC is not None else 0.0,
        "GiaMuTap": float(updated_giaodich.GiaMuTap) if updated_giaodich.GiaMuTap is not None else 0.0,
        "TongTien": float(updated_giaodich.TongTien) if updated_giaodich.TongTien is not None else 0.0
    }

@app.delete("/giaodich/{giaodich_id}", summary="Delete GiaoDich")
def delete_giaodich(giaodich_id: int, db: Session = Depends(get_db)):
    """Delete a GiaoDich by its ID."""
    return crud.delete_giaodich(db, giaodich_id)

# ThanhToan Endpoints
@app.get("/thanhtoan/{thanhtoan_id}", response_model=ThanhToanSchema, summary="Get ThanhToan by ID")
def read_thanhtoan(thanhtoan_id: int, db: Session = Depends(get_db)):
    """Retrieve a ThanhToan by its ID."""
    db_thanhtoan = crud.get_thanhtoan(db, thanhtoan_id)
    if db_thanhtoan is None:
        raise HTTPException(status_code=404, detail="ThanhToan not found")
    return {
        "IDThanhToan": db_thanhtoan.IDThanhToan,
        "IDKhachHang": db_thanhtoan.IDKhachHang,
        "Thang": db_thanhtoan.Thang,
        "TongMuNuoc": float(db_thanhtoan.TongMuNuoc) if db_thanhtoan.TongMuNuoc is not None else 0.0,
        "TongMuTap": float(db_thanhtoan.TongMuTap) if db_thanhtoan.TongMuTap is not None else 0.0,
        "TongThanhToan": float(db_thanhtoan.TongThanhToan) if db_thanhtoan.TongThanhToan is not None else 0.0
    }

@app.get("/thanhtoan/khachhang/{khachhang_id}", response_model=List[ThanhToanSchema], summary="Get ThanhToan by KhachHang")
def read_thanhtoans_by_khachhang(khachhang_id: int, db: Session = Depends(get_db)):
    """Retrieve all payment summaries for a KhachHang."""
    return crud.get_thanhtoans_by_khachhang(db, khachhang_id)

# TriggerLog Endpoint
@app.get("/triggerlog/", response_model=List[TriggerLogSchema], summary="Get TriggerLog entries")
def read_trigger_logs(skip: int = 0, limit: int = 100, db: Session = Depends(get_db)):
    """Retrieve a paginated list of trigger logs."""
    logs = crud.get_trigger_logs(db, skip, limit)
    return [{
        "LogID": log.LogID,
        "Message": log.Message if log.Message else "",
        "LogTime": log.LogTime
    } for log in logs]

# Add these endpoints to main.py under GiaoDich Endpoints
@app.post("/giaodich/mu-tap/", response_model=GiaoDich, summary="Create a new GiaoDich with MuTap")
def create_giaodich_mu_tap(giaodich: GiaoDichMuTapCreate, db: Session = Depends(get_db)):
    """Create a new GiaoDich with MuTap using RFID."""
    logger.info(f"Received request to create GiaoDich MuTap for RFID: {giaodich.RFID}")
    try:
        db_giaodich = crud.create_giaodich_mu_tap(db, giaodich)
        return {
            "IDGiaoDich": db_giaodich.IDGiaoDich,
            "IDKhachHang": db_giaodich.IDKhachHang,
            "NgayGiaoDich": db_giaodich.NgayGiaoDich,
            "ThoiGianGiaoDich": db_giaodich.ThoiGianGiaoDich.strftime("%H:%M:%S"),
            "CongTy": db_giaodich.CongTy,
            "KhoiLuongMuNuoc": float(db_giaodich.MuNuoc) if db_giaodich.MuNuoc is not None else 0.0,
            "DoMuNuoc": float(db_giaodich.TSC) if db_giaodich.TSC is not None else 0.0,
            "KhoiLuongMuTap": float(db_giaodich.MuTap) if db_giaodich.MuTap is not None else 0.0,
            "DoMuTap": float(db_giaodich.DRC) if db_giaodich.DRC is not None else 0.0,
            "MuNuoc": float(db_giaodich.MuNuoc) if db_giaodich.MuNuoc is not None else 0.0,
            "TSC": float(db_giaodich.TSC) if db_giaodich.TSC is not None else 0.0,
            "GiaMuNuoc": float(db_giaodich.GiaMuNuoc) if db_giaodich.GiaMuNuoc is not None else 0.0,
            "MuTap": float(db_giaodich.MuTap) if db_giaodich.MuTap is not None else 0.0,
            "DRC": float(db_giaodich.DRC) if db_giaodich.DRC is not None else 0.0,
            "GiaMuTap": float(db_giaodich.GiaMuTap) if db_giaodich.GiaMuTap is not None else 0.0,
            "TongTien": float(db_giaodich.TongTien) if db_giaodich.TongTien is not None else 0.0
        }
    except HTTPException as e:
        raise e
    except Exception as e:
        logger.error(f"Unexpected error: {str(e)}")
        raise HTTPException(status_code=500, detail="Internal Server Error")

@app.put("/giaodich/mu-nuoc/", response_model=GiaoDich, summary="Update GiaoDich with MuNuoc")
def update_giaodich_mu_nuoc(giaodich: GiaoDichMuNuocCreate, db: Session = Depends(get_db)):
    """Update MuNuoc for the latest GiaoDich using RFID."""
    logger.info(f"Received request to update GiaoDich MuNuoc for RFID: {giaodich.RFID}")
    try:
        db_giaodich = crud.update_giaodich_mu_nuoc(db, giaodich)
        return {
            "IDGiaoDich": db_giaodich.IDGiaoDich,
            "IDKhachHang": db_giaodich.IDKhachHang,
            "NgayGiaoDich": db_giaodich.NgayGiaoDich,
            "ThoiGianGiaoDich": db_giaodich.ThoiGianGiaoDich.strftime("%H:%M:%S"),
            "CongTy": db_giaodich.CongTy,
            "KhoiLuongMuNuoc": float(db_giaodich.MuNuoc) if db_giaodich.MuNuoc is not None else 0.0,
            "DoMuNuoc": float(db_giaodich.TSC) if db_giaodich.TSC is not None else 0.0,
            "KhoiLuongMuTap": float(db_giaodich.MuTap) if db_giaodich.MuTap is not None else 0.0,
            "DoMuTap": float(db_giaodich.DRC) if db_giaodich.DRC is not None else 0.0,
            "MuNuoc": float(db_giaodich.MuNuoc) if db_giaodich.MuNuoc is not None else 0.0,
            "TSC": float(db_giaodich.TSC) if db_giaodich.TSC is not None else 0.0,
            "GiaMuNuoc": float(db_giaodich.GiaMuNuoc) if db_giaodich.GiaMuNuoc is not None else 0.0,
            "MuTap": float(db_giaodich.MuTap) if db_giaodich.MuTap is not None else 0.0,
            "DRC": float(db_giaodich.DRC) if db_giaodich.DRC is not None else 0.0,
            "GiaMuTap": float(db_giaodich.GiaMuTap) if db_giaodich.GiaMuTap is not None else 0.0,
            "TongTien": float(db_giaodich.TongTien) if db_giaodich.TongTien is not None else 0.0
        }
    except HTTPException as e:
        raise e
    except Exception as e:
        logger.error(f"Unexpected error: {str(e)}")
        raise HTTPException(status_code=500, detail="Internal Server Error")

@app.put("/giaodich/tsc-drc/", response_model=GiaoDich, summary="Update GiaoDich with TSC and DRC")
def update_giaodich_tsc_drc(giaodich: GiaoDichTSCDRCCreate, db: Session = Depends(get_db)):
    """Update TSC and DRC for the latest GiaoDich using RFID."""
    logger.info(f"Received request to update GiaoDich TSC/DRC for RFID: {giaodich.RFID}")
    try:
        db_giaodich = crud.update_giaodich_tsc_drc(db, giaodich)
        return {
            "IDGiaoDich": db_giaodich.IDGiaoDich,
            "IDKhachHang": db_giaodich.IDKhachHang,
            "NgayGiaoDich": db_giaodich.NgayGiaoDich,
            "ThoiGianGiaoDich": db_giaodich.ThoiGianGiaoDich.strftime("%H:%M:%S"),
            "CongTy": db_giaodich.CongTy,
            "KhoiLuongMuNuoc": float(db_giaodich.MuNuoc) if db_giaodich.MuNuoc is not None else 0.0,
            "DoMuNuoc": float(db_giaodich.TSC) if db_giaodich.TSC is not None else 0.0,
            "KhoiLuongMuTap": float(db_giaodich.MuTap) if db_giaodich.MuTap is not None else 0.0,
            "DoMuTap": float(db_giaodich.DRC) if db_giaodich.DRC is not None else 0.0,
            "MuNuoc": float(db_giaodich.MuNuoc) if db_giaodich.MuNuoc is not None else 0.0,
            "TSC": float(db_giaodich.TSC) if db_giaodich.TSC is not None else 0.0,
            "GiaMuNuoc": float(db_giaodich.GiaMuNuoc) if db_giaodich.GiaMuNuoc is not None else 0.0,
            "MuTap": float(db_giaodich.MuTap) if db_giaodich.MuTap is not None else 0.0,
            "DRC": float(db_giaodich.DRC) if db_giaodich.DRC is not None else 0.0,
            "GiaMuTap": float(db_giaodich.GiaMuTap) if db_giaodich.GiaMuTap is not None else 0.0,
            "TongTien": float(db_giaodich.TongTien) if db_giaodich.TongTien is not None else 0.0
        }
    except HTTPException as e:
        raise e
    except Exception as e:
        logger.error(f"Unexpected error: {str(e)}")
        raise HTTPException(status_code=500, detail="Internal Server Error")