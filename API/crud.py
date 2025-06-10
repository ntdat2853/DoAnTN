import logging
from typing import Optional
from typing import List
from passlib.context import CryptContext
from sqlalchemy.orm import Session
from sqlalchemy.exc import IntegrityError, DatabaseError, SQLAlchemyError
from sqlalchemy.sql import text
from fastapi import HTTPException
from models import TaiKhoan, GiaoDich, ThanhToan, TriggerLog, KhachHang, QuanLy, QuanTri, GiaMu
from schemas import (
       TaiKhoanCreate, GiaoDichCreate, KhachHangCreate, KhachHangUpdate, QuanLyCreate, QuanLyUpdate,
       QuanTriCreate, QuanTriUpdate, LoginRequest, Token, GiaMuCreate, GiaMuUpdate, PasswordUpdateRequest,
       ThanhToanBase, TriggerLog, GiaoDichMuTapCreate, GiaoDichMuNuocCreate, GiaoDichTSCDRCCreate
   )
import jwt
from datetime import datetime, timedelta, date
from dotenv import load_dotenv
import os

# Setup logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)
pwd_context = CryptContext(schemes=["bcrypt"], deprecated="auto")

# Load environment variables
load_dotenv()

# JWT configuration
SECRET_KEY = os.getenv("JWT_SECRET_KEY")
ALGORITHM = os.getenv("JWT_ALGORITHM")
ACCESS_TOKEN_EXPIRE_MINUTES = os.getenv("ACCESS_TOKEN_EXPIRE_MINUTES")

# Kiểm tra các biến môi trường
if not all([SECRET_KEY, ALGORITHM, ACCESS_TOKEN_EXPIRE_MINUTES]):
    logger.error("Missing required environment variables for JWT")
    raise ValueError(
        "Missing required environment variables: JWT_SECRET_KEY, JWT_ALGORITHM, or ACCESS_TOKEN_EXPIRE_MINUTES")

try:
    ACCESS_TOKEN_EXPIRE_MINUTES = int(ACCESS_TOKEN_EXPIRE_MINUTES)
except (TypeError, ValueError):
    logger.error("ACCESS_TOKEN_EXPIRE_MINUTES must be a valid integer")
    raise ValueError("ACCESS_TOKEN_EXPIRE_MINUTES must be a valid integer")

# CRUD TaiKhoan
def create_taikhoan(db: Session, taikhoan: TaiKhoanCreate):
    logger.info(f"Creating TaiKhoan with TenDangNhap: {taikhoan.TenDangNhap}")
    existing_taikhoan = db.query(TaiKhoan).filter(TaiKhoan.TenDangNhap == taikhoan.TenDangNhap).first()
    if existing_taikhoan:
        logger.warning(f"TenDangNhap {taikhoan.TenDangNhap} already exists")
        raise HTTPException(status_code=400, detail="Tài khoản đã tồn tại")

    taikhoan_dict = taikhoan.dict()
    db_taikhoan = TaiKhoan(**taikhoan_dict)

    try:
        db.add(db_taikhoan)
        db.commit()
        db.refresh(db_taikhoan)
        logger.info(f"Created TaiKhoan with ID: {db_taikhoan.IDTaiKhoan}")
        return db_taikhoan
    except IntegrityError as e:
        db.rollback()
        logger.error(f"IntegrityError: {str(e)}")
        raise HTTPException(status_code=400, detail="Tài khoản đã tồn tại")
    except Exception as e:
        db.rollback()
        logger.error(f"Unexpected error: {str(e)}")
        raise HTTPException(status_code=500, detail="Internal Server Error")

def authenticate_user(db: Session, login_request: LoginRequest):
    logger.info(f"Authenticating user with TenDangNhap: {login_request.TenDangNhap}")
    try:
        user = db.query(TaiKhoan).filter(TaiKhoan.TenDangNhap == login_request.TenDangNhap).first()
        if not user:
            logger.warning(f"TenDangNhap {login_request.TenDangNhap} not found")
            raise HTTPException(status_code=401, detail="Invalid credentials")
        if login_request.MatKhau != user.MatKhau:
            logger.warning(f"Invalid password for TenDangNhap {login_request.TenDangNhap}")
            raise HTTPException(status_code=401, detail="Invalid credentials")
        return user
    except Exception as e:
        logger.error(f"Database error during authentication: {str(e)}")
        raise HTTPException(status_code=500, detail=f"Database error during authentication: {str(e)}")

def create_access_token(data: dict, expires_delta: timedelta = None):
    to_encode = data.copy()
    if expires_delta:
        expire = datetime.utcnow() + expires_delta
    else:
        expire = datetime.utcnow() + timedelta(minutes=ACCESS_TOKEN_EXPIRE_MINUTES)
    to_encode.update({"exp": expire})
    encoded_jwt = jwt.encode(to_encode, SECRET_KEY, algorithm=ALGORITHM)
    return encoded_jwt

def login(db: Session, login_request: LoginRequest):
    try:
        user = authenticate_user(db, login_request)
        access_token_expires = timedelta(minutes=ACCESS_TOKEN_EXPIRE_MINUTES)
        access_token = create_access_token(
            data={"sub": user.TenDangNhap, "vai_tro": user.VaiTro.value},
            expires_delta=access_token_expires
        )
        logger.info(f"Generated JWT for TenDangNhap: {login_request.TenDangNhap}")
        return Token(access_token=access_token, token_type="bearer")
    except HTTPException as e:
        raise e
    except Exception as e:
        logger.error(f"Unexpected error during login: {str(e)}")
        raise HTTPException(status_code=500, detail=f"Internal Server Error: {str(e)}")

def get_taikhoan(db: Session, taikhoan_id: int):
    return db.query(TaiKhoan).filter(TaiKhoan.IDTaiKhoan == taikhoan_id).first()

def get_taikhoan_by_ten_dang_nhap(db: Session, ten_dang_nhap: str):
    logger.info(f"Fetching TaiKhoan with TenDangNhap: {ten_dang_nhap}")
    try:
        taikhoan = db.query(TaiKhoan).filter(TaiKhoan.TenDangNhap == ten_dang_nhap).first()
        if not taikhoan:
            logger.warning(f"TaiKhoan with TenDangNhap {ten_dang_nhap} not found")
            raise HTTPException(status_code=404, detail="TaiKhoan not found")
        return taikhoan
    except Exception as e:
        logger.error(f"Unexpected error: {str(e)}")
        raise HTTPException(status_code=500, detail="Internal Server Error")

def get_taikhoans(db: Session, skip: int = 0, limit: int = 100):
    logger.info(f"Fetching TaiKhoan records with skip={skip}, limit={limit}")
    try:
        result = db.query(TaiKhoan).offset(skip).limit(limit).all()
        logger.info(f"Retrieved {len(result)} TaiKhoan records")
        return result
    except DatabaseError as e:
        logger.error(f"Database error: {str(e)}")
        raise HTTPException(status_code=500, detail="Database Error")
    except Exception as e:
        logger.error(f"Unexpected error: {str(e)}")
        raise HTTPException(status_code=500, detail="Internal Server Error")

def update_taikhoan(db: Session, taikhoan_id: int, taikhoan_update: TaiKhoanCreate):
    logger.info(f"Updating TaiKhoan with ID: {taikhoan_id}")
    db_taikhoan = db.query(TaiKhoan).filter(TaiKhoan.IDTaiKhoan == taikhoan_id).first()
    if not db_taikhoan:
        logger.warning(f"TaiKhoan ID {taikhoan_id} not found")
        raise HTTPException(status_code=404, detail="TaiKhoan not found")

    taikhoan_dict = taikhoan_update.dict()
    for key, value in taikhoan_dict.items():
        setattr(db_taikhoan, key, value)

    try:
        db.commit()
        db.refresh(db_taikhoan)
        logger.info(f"Updated TaiKhoan with ID: {taikhoan_id}")
        return db_taikhoan
    except IntegrityError as e:
        db.rollback()
        logger.error(f"IntegrityError: {str(e)}")
        raise HTTPException(status_code=400, detail="TenDangNhap already exists")
    except Exception as e:
        db.rollback()
        logger.error(f"Unexpected error: {str(e)}")
        raise HTTPException(status_code=500, detail="Internal Server Error")

def update_password(db: Session, ten_dang_nhap: str, password_update: PasswordUpdateRequest):
    logger.info(f"Updating password for TenDangNhap: {ten_dang_nhap}")
    db_taikhoan = db.query(TaiKhoan).filter(TaiKhoan.TenDangNhap == ten_dang_nhap).first()
    if not db_taikhoan:
        logger.warning(f"TaiKhoan with TenDangNhap {ten_dang_nhap} not found")
        raise HTTPException(status_code=404, detail="TaiKhoan not found")

    if db_taikhoan.MatKhau != password_update.current_password:
        logger.warning(f"Invalid current password for TenDangNhap {ten_dang_nhap}")
        raise HTTPException(status_code=400, detail="Mật khẩu hiện tại không đúng")

    db_taikhoan.MatKhau = password_update.new_password
    try:
        db.commit()
        db.refresh(db_taikhoan)
        logger.info(f"Password updated for TenDangNhap: {ten_dang_nhap}")
        return {"detail": "Mật khẩu đã được cập nhật"}
    except Exception as e:
        db.rollback()
        logger.error(f"Unexpected error: {str(e)}")
        raise HTTPException(status_code=500, detail="Internal Server Error")

# CRUD QuanTri
def create_quantri(db: Session, quantri: QuanTriCreate):
    logger.info(f"Creating QuanTri with TenDangNhap: {quantri.TenDangNhap}")
    taikhoan_data = TaiKhoanCreate(
        TenDangNhap=quantri.TenDangNhap,
        MatKhau=quantri.MatKhau,
        NgayTao=date.today(),
        VaiTro="QuanTri"
    )
    db_taikhoan = create_taikhoan(db, taikhoan_data)

    quantri_update = QuanTriUpdate(
        ten_dang_nhap=quantri.TenDangNhap,
        HoVaTen=quantri.HoVaTen,
        SoDienThoai=quantri.SoDienThoai,
        Gmail=quantri.Gmail
    )
    update_quantri_info(db, quantri_update)

    db_quantri = db.query(QuanTri).filter(QuanTri.IDTaiKhoan == db_taikhoan.IDTaiKhoan).first()
    logger.info(f"Created QuanTri with ID: {db_quantri.IDQuanTri}")
    return db_quantri

def get_quantri(db: Session, quantri_id: int):
    logger.info(f"Fetching QuanTri with ID: {quantri_id}")
    return db.query(QuanTri).filter(QuanTri.IDQuanTri == quantri_id).first()

def get_quantri_info(db: Session, ten_dang_nhap: str):
    logger.info(f"Fetching QuanTri info for TenDangNhap: {ten_dang_nhap}")
    try:
        quantri = db.query(QuanTri).join(
            TaiKhoan, QuanTri.IDTaiKhoan == TaiKhoan.IDTaiKhoan
        ).filter(TaiKhoan.TenDangNhap == ten_dang_nhap).first()

        if not quantri:
            logger.warning(f"QuanTri with TenDangNhap {ten_dang_nhap} not found")
            raise HTTPException(status_code=404, detail="QuanTri not found")

        return {
            "HoVaTen": quantri.HoVaTen if quantri.HoVaTen else "",
            "SoDienThoai": quantri.SoDienThoai if quantri.SoDienThoai else "",
            "Gmail": quantri.Gmail if quantri.Gmail else ""
        }
    except Exception as e:
        logger.error(f"Unexpected error while fetching QuanTri info: {str(e)}")
        raise HTTPException(status_code=500, detail="Internal Server Error")

def update_quantri_info(db: Session, quantri_update: QuanTriUpdate):
    logger.info(f"Updating QuanTri info for TenDangNhap: {quantri_update.ten_dang_nhap}")
    try:
        db.execute(
            text("CALL UpdateQuanTriInfo(:ten_dang_nhap, :ho_va_ten, :so_dien_thoai, :gmail)"),
            {
                "ten_dang_nhap": quantri_update.ten_dang_nhap,
                "ho_va_ten": quantri_update.HoVaTen,
                "so_dien_thoai": quantri_update.SoDienThoai,
                "gmail": quantri_update.Gmail
            }
        )
        db.commit()
        logger.info(f"Updated QuanTri info for TenDangNhap: {quantri_update.ten_dang_nhap}")
    except Exception as e:
        db.rollback()
        logger.error(f"Unexpected error: {str(e)}")
        raise HTTPException(status_code=500, detail="Internal Server Error")

# CRUD QuanLy
def create_quanly(db: Session, quanly: QuanLyCreate):
    logger.info(f"Creating QuanLy with TenDangNhap: {quanly.TenDangNhap}")
    taikhoan_data = TaiKhoanCreate(
        TenDangNhap=quanly.TenDangNhap,
        MatKhau=quanly.MatKhau,
        NgayTao=date.today(),
        VaiTro="QuanLy"
    )
    db_taikhoan = create_taikhoan(db, taikhoan_data)

    quanly_update = QuanLyUpdate(
        ten_dang_nhap=quanly.TenDangNhap,
        HoVaTen=quanly.HoVaTen,
        SoDienThoai=quanly.SODienThoai,
        Gmail=quanly.Gmail,
        CongTy=quanly.CongTy
    )
    update_quanly_info(db, quanly_update)

    db_quanly = db.query(QuanLy).filter(QuanLy.IDTaiKhoan == db_taikhoan.IDTaiKhoan).first()
    logger.info(f"Created QuanLy with ID: {db_quanly.IDQuanLy}")
    return db_quanly

def get_quanly(db: Session, quanly_id: int):
    logger.info(f"Fetching QuanLy with ID: {quanly_id}")
    return db.query(QuanLy).filter(QuanLy.IDQuanLy == quanly_id).first()

def get_quanly(db: Session, skip: int = 0, limit: int = 100):
    logger.info(f"Fetching QuanLy records with skip={skip}, limit={limit}")
    try:
        result = db.query(QuanLy).offset(skip).limit(limit).all()
        logger.info(f"Retrieved {len(result)} QuanLy records")
        return result
    except Exception as e:
        logger.error(f"Error fetching QuanLy: {str(e)}")
        raise HTTPException(status_code=500, detail="Internal Server Error")

def get_quanly_info(db: Session, ten_dang_nhap: str):
    logger.info(f"Fetching QuanLy info for TenDangNhap: {ten_dang_nhap}")
    try:
        quanly = db.query(QuanLy).join(
            TaiKhoan, QuanLy.IDTaiKhoan == TaiKhoan.IDTaiKhoan
        ).filter(TaiKhoan.TenDangNhap == ten_dang_nhap).first()

        if not quanly:
            logger.warning(f"QuanLy with TenDangNhap {ten_dang_nhap} not found")
            raise HTTPException(status_code=404, detail="QuanLy not found")

        customer_count = db.query(KhachHang).filter(KhachHang.CongTy == quanly.CongTy).count()
        return {
            "HoVaTen": quanly.HoVaTen if quanly.HoVaTen else "",
            "SoDienThoai": quanly.SoDienThoai if quanly.SoDienThoai else "",
            "Gmail": quanly.Gmail if quanly.Gmail else "",
            "CongTy": quanly.CongTy if quanly.CongTy else "",
            "CustomerCount": customer_count
        }
    except Exception as e:
        logger.error(f"Unexpected error while fetching QuanLy info: {str(e)}")
        raise HTTPException(status_code=500, detail="Internal Server Error")

def update_quanly_info(db: Session, quanly_update: QuanLyUpdate):
    logger.info(f"Updating QuanLy info for TenDangNhap: {quanly_update.ten_dang_nhap}")
    try:
        db.execute(
            text("CALL UpdateQuanLyInfo(:ten_dang_nhap, :ho_va_ten, :so_dien_thoai, :gmail, :cong_ty)"),
            {
                "ten_dang_nhap": quanly_update.ten_dang_nhap,
                "ho_va_ten": quanly_update.HoVaTen,
                "so_dien_thoai": quanly_update.SoDienThoai,
                "gmail": quanly_update.Gmail,
                "cong_ty": quanly_update.CongTy
            }
        )
        db.commit()
        logger.info(f"Updated QuanLy info for TenDangNhap: {quanly_update.ten_dang_nhap}")
    except IntegrityError as e:
        db.rollback()
        logger.error(f"IntegrityError: {str(e)}")
        error_detail = str(e)
        if "CongTy" in error_detail:
            raise HTTPException(status_code=400, detail="Tên công ty đã tồn tại")
        raise HTTPException(status_code=400, detail="IntegrityError occurred")
    except Exception as e:
        db.rollback()
        logger.error(f"Unexpected error: {str(e)}")
        raise HTTPException(status_code=500, detail="Internal Server Error")

def delete_quanly_by_id_taikhoan(db: Session, taikhoan_id: int):
    logger.info(f"Deleting QuanLy with IDTaiKhoan: {taikhoan_id}")
    quanly = db.query(QuanLy).filter(QuanLy.IDTaiKhoan == taikhoan_id).first()
    if not quanly:
        logger.warning(f"QuanLy with IDTaiKhoan {taikhoan_id} not found")
        raise HTTPException(status_code=404, detail="QuanLy not found")

    try:
        # Xóa tất cả khách hàng thuộc công ty của quản lý
        khach_hangs = db.query(KhachHang).filter(KhachHang.CongTy == quanly.CongTy).all()
        for khach_hang in khach_hangs:
            db.query(GiaoDich).filter(GiaoDich.IDKhachHang == khach_hang.IDKhachHang).delete()
            db.query(ThanhToan).filter(ThanhToan.IDKhachHang == khach_hang.IDKhachHang).delete()
            db.query(KhachHang).filter(KhachHang.IDKhachHang == khach_hang.IDKhachHang).delete()
            db.query(TaiKhoan).filter(TaiKhoan.IDTaiKhoan == khach_hang.IDTaiKhoan).delete()

        # Xóa QuanLy và TaiKhoan
        db.query(QuanLy).filter(QuanLy.IDTaiKhoan == taikhoan_id).delete()
        db.query(TaiKhoan).filter(TaiKhoan.IDTaiKhoan == taikhoan_id).delete()

        db.commit()
        logger.info(f"Deleted QuanLy with IDTaiKhoan: {taikhoan_id} and associated customers")
        return {"detail": f"Deleted QuanLy with IDTaiKhoan: {taikhoan_id}"}
    except Exception as e:
        db.rollback()
        logger.error(f"Unexpected error while deleting QuanLy: {str(e)}")
        raise HTTPException(status_code=500, detail=f"Unexpected error: {str(e)}")

def create_quanly_with_phone(db: Session, quanly: QuanLyCreate):
    logger.info(f"Creating QuanLy with phone: {quanly.SODienThoai}")
    if not quanly.SODienThoai:
        logger.warning("SODienThoai is None or empty")
        raise HTTPException(status_code=400, detail="Số điện thoại không được để trống")

    existing_taikhoan = db.query(TaiKhoan).filter(TaiKhoan.TenDangNhap == quanly.SODienThoai).first()
    if existing_taikhoan:
        logger.warning(f"TenDangNhap {quanly.SODienThoai} already exists")
        raise HTTPException(status_code=400, detail="Số điện thoại đã được sử dụng")

    taikhoan_data = TaiKhoanCreate(
        TenDangNhap=quanly.SODienThoai,
        MatKhau=quanly.SODienThoai,  # Sử dụng SODienThoai làm mật khẩu
        NgayTao=date.today(),
        VaiTro="QuanLy"
    )
    db_taikhoan = create_taikhoan(db, taikhoan_data)

    quanly_update = QuanLyUpdate(
        ten_dang_nhap=quanly.SODienThoai,
        HoVaTen=quanly.HoVaTen,
        SoDienThoai=quanly.SODienThoai,
        Gmail=quanly.Gmail,
        CongTy=quanly.CongTy
    )
    update_quanly_info(db, quanly_update)

    db_quanly = db.query(QuanLy).filter(QuanLy.IDTaiKhoan == db_taikhoan.IDTaiKhoan).first()
    logger.info(f"Created QuanLy with ID: {db_quanly.IDQuanLy}")
    return db_quanly

def delete_quanly_by_congty(db: Session, cong_ty_list: List[str]):
    logger.info(f"Deleting QuanLy for companies: {cong_ty_list}")
    for cong_ty in cong_ty_list:
        quanly = db.query(QuanLy).filter(QuanLy.CongTy == cong_ty).first()
        if not quanly:
            logger.warning(f"QuanLy with CongTy {cong_ty} not found")
            continue

        # Xóa khách hàng và dữ liệu liên quan
        khach_hangs = db.query(KhachHang).filter(KhachHang.CongTy == cong_ty).all()
        for khach_hang in khach_hangs:
            db.query(GiaoDich).filter(GiaoDich.IDKhachHang == khach_hang.IDKhachHang).delete()
            db.query(ThanhToan).filter(ThanhToan.IDKhachHang == khach_hang.IDKhachHang).delete()
            db.query(KhachHang).filter(KhachHang.IDKhachHang == khach_hang.IDKhachHang).delete()
            db.query(TaiKhoan).filter(TaiKhoan.IDTaiKhoan == khach_hang.IDTaiKhoan).delete()

        # Xóa giá mủ
        db.query(GiaMu).filter(GiaMu.CongTy == cong_ty).delete()

        # Xóa QuanLy và TaiKhoan
        db.query(QuanLy).filter(QuanLy.CongTy == cong_ty).delete()
        db.query(TaiKhoan).filter(TaiKhoan.IDTaiKhoan == quanly.IDTaiKhoan).delete()

    try:
        db.commit()
        logger.info(f"Deleted QuanLy for companies: {cong_ty_list}")
        return {"detail": "Deleted companies successfully"}
    except Exception as e:
        db.rollback()
        logger.error(f"Unexpected error while deleting companies: {str(e)}")
        raise HTTPException(status_code=500, detail=f"Internal Server Error: {str(e)}")

# CRUD KhachHang
def create_khachhang(db: Session, khachhang: KhachHangCreate):
    logger.info(f"Creating KhachHang with TenDangNhap: {khachhang.TenDangNhap}")
    taikhoan_data = TaiKhoanCreate(
        TenDangNhap=khachhang.TenDangNhap,
        MatKhau=khachhang.MatKhau,
        NgayTao=date.today(),
        VaiTro="KhachHang"
    )
    db_taikhoan = create_taikhoan(db, taikhoan_data)

    khachhang_update = KhachHangUpdate(
        ten_dang_nhap=khachhang.TenDangNhap,
        HoVaTen=khachhang.HoVaTen,
        Gmail=khachhang.Gmail,
        RFID=khachhang.RFID,
        SoDienThoai=khachhang.SoDienThoai,
        CongTy=khachhang.CongTy,
        SoTaiKhoan=khachhang.SoTaiKhoan,
        NganHang=khachhang.NganHang
    )
    update_khachhang_info(db, khachhang_update)

    db_khachhang = db.query(KhachHang).filter(KhachHang.IDTaiKhoan == db_taikhoan.IDTaiKhoan).first()
    logger.info(f"Created KhachHang with ID: {db_khachhang.IDKhachHang}")
    return db_khachhang

def get_khachhang(db: Session, khachhang_id: int):
    logger.info(f"Fetching KhachHang with ID: {khachhang_id}")
    khachhang = db.query(KhachHang).filter(KhachHang.IDKhachHang == khachhang_id).first()
    if not khachhang:
        logger.warning(f"KhachHang with ID {khachhang_id} not found")
        raise HTTPException(status_code=404, detail="KhachHang not found")

    # Lấy TenDangNhap từ bảng TaiKhoan thông qua IDTaiKhoan
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
        "TenDangNhap": taikhoan.TenDangNhap if taikhoan else ""
    }
    return khachhang_dict

def get_khachhang_info(db: Session, ten_dang_nhap: str):
    logger.info(f"Fetching KhachHang info for TenDangNhap: {ten_dang_nhap}")
    try:
        khachhang = db.query(KhachHang).join(
            TaiKhoan, KhachHang.IDTaiKhoan == TaiKhoan.IDTaiKhoan
        ).filter(TaiKhoan.TenDangNhap == ten_dang_nhap).first()

        if not khachhang:
            logger.warning(f"KhachHang with TenDangNhap {ten_dang_nhap} not found")
            raise HTTPException(status_code=404, detail="KhachHang not found")

        return {
            "IDKhachHang": khachhang.IDKhachHang,
            "HoVaTen": khachhang.HoVaTen if khachhang.HoVaTen else "",
            "SoDienThoai": khachhang.SoDienThoai if khachhang.SoDienThoai else "",
            "Gmail": khachhang.Gmail if khachhang.Gmail else "",
            "CongTy": khachhang.CongTy if khachhang.CongTy else "",
            "SoTaiKhoan": khachhang.SoTaiKhoan if khachhang.SoTaiKhoan else "",
            "NganHang": khachhang.NganHang if khachhang.NganHang else "",
            "RFID": khachhang.RFID if khachhang.RFID else ""
        }
    except Exception as e:
        logger.error(f"Unexpected error while fetching KhachHang info: {str(e)}")
        raise HTTPException(status_code=500, detail="Internal Server Error")

def update_khachhang_info(db: Session, khachhang_update: KhachHangUpdate):
    logger.info(f"Updating KhachHang info for TenDangNhap: {khachhang_update.ten_dang_nhap}")
    if khachhang_update.CongTy and not db.query(QuanLy).filter(QuanLy.CongTy == khachhang_update.CongTy).first():
        logger.warning(f"CongTy {khachhang_update.CongTy} not found")
        raise HTTPException(status_code=400, detail="CongTy does not exist")

    try:
        db.execute(
            text(
                "CALL UpdateKhachHangInfo(:ten_dang_nhap, :ho_va_ten, :gmail, :rfid, :so_dien_thoai, :cong_ty, :so_tai_khoan, :ngan_hang)"),
            {
                "ten_dang_nhap": khachhang_update.ten_dang_nhap,
                "ho_va_ten": khachhang_update.HoVaTen,
                "gmail": khachhang_update.Gmail,
                "rfid": khachhang_update.RFID,
                "so_dien_thoai": khachhang_update.SoDienThoai,
                "cong_ty": khachhang_update.CongTy,
                "so_tai_khoan": khachhang_update.SoTaiKhoan,
                "ngan_hang": khachhang_update.NganHang
            }
        )
        db.commit()
        logger.info(f"Updated KhachHang info for TenDangNhap: {khachhang_update.ten_dang_nhap}")
    except Exception as e:
        db.rollback()
        logger.error(f"Unexpected error: {str(e)}")
        raise HTTPException(status_code=500, detail="Internal Server Error")

def delete_khach_hang_by_id_taikhoan(db: Session, taikhoan_id: int):
    logger.info(f"Deleting KhachHang with IDTaiKhoan: {taikhoan_id}")
    khach_hang = db.query(KhachHang).filter(KhachHang.IDTaiKhoan == taikhoan_id).first()
    if not khach_hang:
        logger.warning(f"KhachHang with IDTaiKhoan {taikhoan_id} not found")
        raise HTTPException(status_code=404, detail="KhachHang not found")

    # Xóa KhachHang
    db.query(KhachHang).filter(KhachHang.IDTaiKhoan == taikhoan_id).delete()

    # Xóa TaiKhoan
    taikhoan = db.query(TaiKhoan).filter(TaiKhoan.IDTaiKhoan == taikhoan_id).first()
    if not taikhoan:
        logger.warning(f"TaiKhoan with IDTaiKhoan {taikhoan_id} not found")
        raise HTTPException(status_code=404, detail="TaiKhoan not found")
    db.query(TaiKhoan).filter(TaiKhoan.IDTaiKhoan == taikhoan_id).delete()

    try:
        db.commit()
        logger.info(f"Deleted KhachHang with IDTaiKhoan: {taikhoan_id}")
        return {"detail": "Deleted KhachHang successfully"}
    except Exception as e:
        db.rollback()
        logger.error(f"Unexpected error: {str(e)}")
        raise HTTPException(status_code=500, detail="Internal Server Error")

# CRUD GiaoDich
def create_giaodich(db: Session, giaodich: GiaoDichCreate):
    logger.info(f"Creating GiaoDich with IDKhachHang: {giaodich.IDKhachHang}")
    khach_hang = db.query(KhachHang).filter(KhachHang.IDKhachHang == giaodich.IDKhachHang).first()
    if not khach_hang:
        logger.warning(f"IDKhachHang {giaodich.IDKhachHang} not found")
        raise HTTPException(status_code=400, detail="IDKhachHang does not exist")

    # Kiểm tra giá mủ (trigger before_giaodich_insert sẽ xử lý việc gán GiaMuNuoc, GiaMuTap)
    if not db.query(GiaMu).filter(
            GiaMu.NgayThietLap == giaodich.NgayGiaoDich,
            GiaMu.CongTy == giaodich.CongTy
    ).first():
        logger.warning(f"No price defined for date {giaodich.NgayGiaoDich} and CongTy {giaodich.CongTy}")
        raise HTTPException(status_code=400,
                            detail=f"No price defined for date {giaodich.NgayGiaoDich} and CongTy {giaodich.CongTy}")

    giaodich_dict = {
        "IDKhachHang": giaodich.IDKhachHang,
        "NgayGiaoDich": giaodich.NgayGiaoDich,
        "CongTy": giaodich.CongTy,
        "ThoiGianGiaoDich": giaodich.ThoiGianGiaoDich,
        "MuNuoc": giaodich.KhoiLuongMuNuoc,
        "TSC": giaodich.DoMuNuoc,
        "MuTap": giaodich.KhoiLuongMuTap,
        "DRC": giaodich.DoMuTap
    }
    db_giaodich = GiaoDich(**giaodich_dict)
    try:
        db.add(db_giaodich)
        db.commit()
        db.refresh(db_giaodich)
        logger.info(f"Created GiaoDich with ID: {db_giaodich.IDGiaoDich}")
        return db_giaodich
    except Exception as e:
        db.rollback()
        logger.error(f"Unexpected error: {str(e)}")
        raise HTTPException(status_code=500, detail="Internal Server Error")

def get_giaodich(db: Session, giaodich_id: int):
    return db.query(GiaoDich).filter(GiaoDich.IDGiaoDich == giaodich_id).first()

def get_giaodichs_by_khachhang(db: Session, khachhang_id: int):
    logger.info(f"Fetching GiaoDich records for IDKhachHang: {khachhang_id}")
    try:
        giao_dichs = db.query(GiaoDich).filter(GiaoDich.IDKhachHang == khachhang_id).all()
        if not giao_dichs:
            logger.warning(f"No GiaoDich records found for IDKhachHang {khachhang_id}")
            return []

        # Ánh xạ dữ liệu để khớp với schema và giao diện
        result = []
        for giao_dich in giao_dichs:
            giao_dich_dict = {
                "IDGiaoDich": giao_dich.IDGiaoDich,
                "IDKhachHang": giao_dich.IDKhachHang,
                "NgayGiaoDich": giao_dich.NgayGiaoDich.strftime("%Y-%m-%d"),
                "ThoiGianGiaoDich": giao_dich.ThoiGianGiaoDich.strftime("%H:%M:%S"),
                "CongTy": giao_dich.CongTy,
                "KhoiLuongMuNuoc": float(giao_dich.MuNuoc) if giao_dich.MuNuoc is not None else 0.0,
                "DoMuNuoc": float(giao_dich.TSC) if giao_dich.TSC is not None else 0.0,
                "KhoiLuongMuTap": float(giao_dich.MuTap) if giao_dich.MuTap is not None else 0.0,
                "DoMuTap": float(giao_dich.DRC) if giao_dich.DRC is not None else 0.0,
                "MuNuoc": float(giao_dich.MuNuoc) if giao_dich.MuNuoc is not None else 0.0,
                "TSC": float(giao_dich.TSC) if giao_dich.TSC is not None else 0.0,
                "GiaMuNuoc": float(giao_dich.GiaMuNuoc) if giao_dich.GiaMuNuoc is not None else 0.0,
                "MuTap": float(giao_dich.MuTap) if giao_dich.MuTap is not None else 0.0,
                "DRC": float(giao_dich.DRC) if giao_dich.DRC is not None else 0.0,
                "GiaMuTap": float(giao_dich.GiaMuTap) if giao_dich.GiaMuTap is not None else 0.0,
                "TongTien": float(giao_dich.TongTien) if giao_dich.TongTien is not None else 0.0
            }
            result.append(giao_dich_dict)

        logger.info(f"Retrieved {len(result)} GiaoDich records for IDKhachHang: {khachhang_id}")
        return result
    except Exception as e:
        logger.error(f"Unexpected error while fetching GiaoDich: {str(e)}")
        raise HTTPException(status_code=500, detail="Internal Server Error")

def update_giaodich(db: Session, giaodich_id: int, giaodich_update: GiaoDichCreate):
    logger.info(f"Updating GiaoDich with ID: {giaodich_id}")
    db_giaodich = db.query(GiaoDich).filter(GiaoDich.IDGiaoDich == giaodich_id).first()
    if not db_giaodich:
        logger.warning(f"GiaoDich ID {giaodich_id} not found")
        raise HTTPException(status_code=404, detail="GiaoDich not found")

    khach_hang = db.query(KhachHang).filter(KhachHang.IDKhachHang == giaodich_update.IDKhachHang).first()
    if not khach_hang:
        logger.warning(f"IDKhachHang {giaodich_update.IDKhachHang} not found")
        raise HTTPException(status_code=400, detail="IDKhachHang does not exist")

    if not db.query(GiaMu).filter(
            GiaMu.NgayThietLap == giaodich_update.NgayGiaoDich,
            GiaMu.CongTy == giaodich_update.CongTy
    ).first():
        logger.warning(f"No price defined for date {giaodich_update.NgayGiaoDich} and CongTy {giaodich_update.CongTy}")
        raise HTTPException(status_code=400,
                            detail=f"No price defined for date {giaodich_update.NgayGiaoDich} and CongTy {giaodich_update.CongTy}")

    db_giaodich.IDKhachHang = giaodich_update.IDKhachHang
    db_giaodich.NgayGiaoDich = giaodich_update.NgayGiaoDich
    db_giaodich.CongTy = giaodich_update.CongTy
    db_giaodich.ThoiGianGiaoDich = giaodich_update.ThoiGianGiaoDich
    db_giaodich.MuNuoc = giaodich_update.KhoiLuongMuNuoc
    db_giaodich.TSC = giaodich_update.DoMuNuoc
    db_giaodich.MuTap = giaodich_update.KhoiLuongMuTap
    db_giaodich.DRC = giaodich_update.DoMuTap

    try:
        db.commit()
        db.refresh(db_giaodich)
        logger.info(f"Updated GiaoDich with ID: {giaodich_id}")
        return db_giaodich
    except Exception as e:
        db.rollback()
        logger.error(f"Unexpected error: {str(e)}")
        raise HTTPException(status_code=500, detail="Internal Server Error")

def delete_giaodich(db: Session, giaodich_id: int):
    logger.info(f"Deleting GiaoDich with ID: {giaodich_id}")
    db_giaodich = db.query(GiaoDich).filter(GiaoDich.IDGiaoDich == giaodich_id).first()
    if not db_giaodich:
        logger.warning(f"GiaoDich ID {giaodich_id} not found")
        raise HTTPException(status_code=400, detail="GiaoDich not found")

    try:
        db.delete(db_giaodich)
        db.commit()
        logger.info(f"Deleted GiaoDich with ID: {giaodich_id}")
        return {"detail": "GiaoDich deleted"}
    except Exception as e:
        db.rollback()
        logger.error(f"Unexpected error: {str(e)}")
        raise HTTPException(status_code=500, detail="Internal Server Error")

# CRUD ThanhToan
def get_thanhtoan(db: Session, thanhtoan_id: int):
    return db.query(ThanhToan).filter(ThanhToan.IDThanhToan == thanhtoan_id).first()

def get_thanhtoans_by_khachhang(db: Session, khachhang_id: int):
    logger.info(f"Fetching ThanhToan records for IDKhachHang: {khachhang_id}")
    try:
        thanh_toans = db.query(ThanhToan).filter(ThanhToan.IDKhachHang == khachhang_id).all()
        if not thanh_toans:
            logger.warning(f"No ThanhToan records found for IDKhachHang {khachhang_id}")
            return []

        # Ánh xạ dữ liệu để khớp với schema và giao diện
        result = []
        for thanh_toan in thanh_toans:
            # Định dạng Thang thành MM/YYYY
            thang = thanh_toan.Thang.replace("-", "/")
            thanh_toan_dict = {
                "IDThanhToan": thanh_toan.IDThanhToan,
                "IDKhachHang": thanh_toan.IDKhachHang,
                "Thang": thang,
                "TongMuNuoc": float(thanh_toan.TongMuNuoc) if thanh_toan.TongMuNuoc is not None else 0.0,
                "TongMuTap": float(thanh_toan.TongMuTap) if thanh_toan.TongMuTap is not None else 0.0,
                "TongThanhToan": float(thanh_toan.TongThanhToan) if thanh_toan.TongThanhToan is not None else 0.0
            }
            result.append(thanh_toan_dict)

        logger.info(f"Retrieved {len(result)} ThanhToan records for IDKhachHang: {khachhang_id}")
        return result
    except Exception as e:
        logger.error(f"Unexpected error while fetching ThanhToan: {str(e)}")
        raise HTTPException(status_code=500, detail="Internal Server Error")

def update_thanh_toan(db: Session, thanhtoan_id: int, thanhtoan_update: ThanhToanBase):
    logger.info(f"Updating ThanhToan with ID: {thanhtoan_id}")
    db_thanh_toan = db.query(ThanhToan).filter(ThanhToan.IDThanhToan == thanhtoan_id).first()
    if not db_thanh_toan:
        logger.warning(f"ThanhToan ID {thanhtoan_id} not found")
        raise HTTPException(status_code=404, detail="ThanhToan not found")

    khach_hang = db.query(KhachHang).filter(KhachHang.IDKhachHang == thanhtoan_update.IDKhachHang).first()
    if not khach_hang:
        logger.warning(f"IDKhachHang {thanhtoan_update.IDKhachHang} not found")
        raise HTTPException(status_code=400, detail="IDKhachHang does not exist")

    db_thanh_toan.IDKhachHang = thanhtoan_update.IDKhachHang
    db_thanh_toan.Thang = thanhtoan_update.Thang
    db_thanh_toan.TongMuNuoc = thanhtoan_update.TongMuNuoc
    db_thanh_toan.TongMuTap = thanhtoan_update.TongMuTap
    db_thanh_toan.TongThanhToan = thanhtoan_update.TongThanhToan

    try:
        db.commit()
        db.refresh(db_thanh_toan)
        logger.info(f"Updated ThanhToan with ID: {thanhtoan_id}")
        return db_thanh_toan
    except IntegrityError as e:
        db.rollback()
        logger.error(f"IntegrityError: {str(e)}")
        raise HTTPException(status_code=400, detail="Duplicate entry for IDKhachHang and Thang")
    except Exception as e:
        db.rollback()
        logger.error(f"Unexpected error: {str(e)}")
        raise HTTPException(status_code=500, detail="Internal Server Error")

def delete_thanh_toan(db: Session, thanhtoan_id: int):
    logger.info(f"Deleting ThanhToan with ID: {thanhtoan_id}")
    db_thanh_toan = db.query(ThanhToan).filter(ThanhToan.IDThanhToan == thanhtoan_id).first()
    if not db_thanh_toan:
        logger.warning(f"ThanhToan ID {thanhtoan_id} not found")
        raise HTTPException(status_code=400, detail="ThanhToan not found")

    try:
        db.delete(db_thanh_toan)
        db.commit()
        logger.info(f"Deleted ThanhToan with ID: {thanhtoan_id}")
        return {"detail": "ThanhToan deleted"}
    except Exception as e:
        db.rollback()
        logger.error(f"Unexpected error: {str(e)}")
        raise HTTPException(status_code=500, detail="Internal Server Error")

# CRUD GiaMu
def create_gia_mu(db: Session, gia_mu: GiaMuCreate):
    logger.info(f"Creating/Updating GiaMu for date: {gia_mu.NgayThietLap}, CongTy: {gia_mu.CongTy}")
    if gia_mu.GiaMuNuoc == gia_mu.GiaMuTap:
        logger.warning("GiaMuNuoc and GiaMuTap cannot be the same")
        raise HTTPException(status_code=400, detail="GiaMuNuoc and GiaMuTap must be different")

    if not db.query(QuanLy).filter(QuanLy.CongTy == gia_mu.CongTy).first():
        logger.warning(f"CongTy {gia_mu.CongTy} not found")
        raise HTTPException(status_code=400, detail="CongTy does not exist")

    try:
        ngay_thiet_lap = datetime.strptime(gia_mu.NgayThietLap, "%Y-%m-%d").date()
    except ValueError as e:
        logger.error(f"Invalid date format for NgayThietLap: {str(e)}")
        raise HTTPException(status_code=400, detail="Invalid date format for NgayThietLap. Use YYYY-MM-DD")

    # Tìm bản ghi giá mủ mới nhất cho ngày và công ty
    existing_gia_mu = db.query(GiaMu).filter(
        GiaMu.NgayThietLap == ngay_thiet_lap,
        GiaMu.CongTy == gia_mu.CongTy
    ).order_by(GiaMu.IDGiaMu.desc()).first()

    try:
        if existing_gia_mu:
            # Cập nhật bản ghi hiện có
            existing_gia_mu.GiaMuNuoc = gia_mu.GiaMuNuoc
            existing_gia_mu.GiaMuTap = gia_mu.GiaMuTap
            db.commit()
            db.refresh(existing_gia_mu)
            logger.info(f"Updated GiaMu with ID: {existing_gia_mu.IDGiaMu}")
        else:
            # Tạo bản ghi mới
            gia_mu_dict = {
                "NgayThietLap": ngay_thiet_lap,
                "CongTy": gia_mu.CongTy,
                "GiaMuNuoc": gia_mu.GiaMuNuoc,
                "GiaMuTap": gia_mu.GiaMuTap
            }
            db_gia_mu = GiaMu(**gia_mu_dict)
            db.add(db_gia_mu)
            db.commit()
            db.refresh(db_gia_mu)
            logger.info(f"Created GiaMu with ID: {db_gia_mu.IDGiaMu}")
            existing_gia_mu = db_gia_mu

        return {
            "IDGiaMu": existing_gia_mu.IDGiaMu,
            "NgayThietLap": existing_gia_mu.NgayThietLap.strftime("%Y-%m-%d"),
            "CongTy": existing_gia_mu.CongTy,
            "GiaMuNuoc": float(existing_gia_mu.GiaMuNuoc),
            "GiaMuTap": float(existing_gia_mu.GiaMuTap)
        }
    except Exception as e:
        db.rollback()
        logger.error(f"Unexpected error: {str(e)}")
        raise HTTPException(status_code=500, detail="Internal Server Error")


def create_khach_hang(db: Session, khach_hang: KhachHangCreate):
    logger.info(f"Creating KhachHang with TenDangNhap: {khach_hang.TenDangNhap}")

    # Kiểm tra công ty hợp lệ
    if khach_hang.CongTy and not db.query(QuanLy).filter(QuanLy.CongTy == khach_hang.CongTy).first():
        logger.warning(f"CongTy {khach_hang.CongTy} not found")
        raise HTTPException(status_code=400, detail="Công ty không tồn tại")

    # Tạo tài khoản
    db_taikhoan = TaiKhoan(
        TenDangNhap=khach_hang.TenDangNhap,
        MatKhau=khach_hang.MatKhau,  # Lưu ý: Nên mã hóa mật khẩu trong thực tế
        NgayTao=khach_hang.NgayTao,
        VaiTro="KhachHang"
    )
    try:
        db.add(db_taikhoan)
        db.commit()
        db.refresh(db_taikhoan)
        logger.info(f"Created TaiKhoan with ID: {db_taikhoan.IDTaiKhoan}")
    except IntegrityError as e:
        db.rollback()
        logger.error(f"IntegrityError: {str(e)}")
        raise HTTPException(status_code=400, detail="Số điện thoại đã được sử dụng")
    except Exception as e:
        db.rollback()
        logger.error(f"Unexpected error: {str(e)}")
        raise HTTPException(status_code=500, detail="Lỗi hệ thống khi tạo tài khoản")

    # Cập nhật thông tin khách hàng
    db_khach_hang = db.query(KhachHang).filter(KhachHang.IDTaiKhoan == db_taikhoan.IDTaiKhoan).first()
    if not db_khach_hang:
        db.rollback()
        logger.error(f"KhachHang not found for IDTaiKhoan: {db_taikhoan.IDTaiKhoan}")
        raise HTTPException(status_code=500, detail="Không tìm thấy thông tin khách hàng sau khi tạo tài khoản")

    db_khach_hang.HoVaTen = khach_hang.HoVaTen
    db_khach_hang.Gmail = khach_hang.Gmail
    db_khach_hang.RFID = khach_hang.RFID
    db_khach_hang.SoDienThoai = khach_hang.SoDienThoai
    db_khach_hang.CongTy = khach_hang.CongTy
    db_khach_hang.SoTaiKhoan = khach_hang.SoTaiKhoan
    db_khach_hang.NganHang = khach_hang.NganHang

    try:
        db.commit()
        db.refresh(db_khach_hang)
        logger.info(f"Updated KhachHang info for TenDangNhap: {khach_hang.TenDangNhap}")
    except Exception as e:
        db.rollback()
        logger.error(f"Unexpected error: {str(e)}")
        raise HTTPException(status_code=500, detail="Lỗi hệ thống khi cập nhật thông tin khách hàng")

    return {
        "IDKhachHang": db_khach_hang.IDKhachHang,
        "ten_dang_nhap": db_taikhoan.TenDangNhap,
        "HoVaTen": db_khach_hang.HoVaTen,
        "Gmail": db_khach_hang.Gmail,
        "RFID": db_khach_hang.RFID,
        "SoDienThoai": db_khach_hang.SoDienThoai,
        "CongTy": db_khach_hang.CongTy,
        "SoTaiKhoan": db_khach_hang.SoTaiKhoan,
        "NganHang": db_khach_hang.NganHang
    }

def update_gia_mu(db: Session, gia_mu_update: GiaMuUpdate):
    logger.info(f"Updating GiaMu for date: {gia_mu_update.NgayThietLap}, CongTy: {gia_mu_update.CongTy}")
    if gia_mu_update.GiaMuNuoc == gia_mu_update.GiaMuTap:
        logger.warning("GiaMuNuoc and GiaMuTap cannot be the same")
        raise HTTPException(status_code=400, detail="GiaMuNuoc and GiaMuTap must be different")

    if not db.query(QuanLy).filter(QuanLy.CongTy == gia_mu_update.CongTy).first():
        logger.warning(f"CongTy {gia_mu_update.CongTy} not found")
        raise HTTPException(status_code=400, detail="CongTy does not exist")

    # Chuyển đổi NgayThietLap từ str sang date
    try:
        ngay_thiet_lap = datetime.strptime(gia_mu_update.NgayThietLap, "%Y-%m-%d").date()
    except ValueError as e:
        logger.error(f"Invalid date format for NgayThietLap: {str(e)}")
        raise HTTPException(status_code=400, detail="Invalid date format for NgayThietLap. Use YYYY-MM-DD")

    db_gia_mu = db.query(GiaMu).filter(
        GiaMu.NgayThietLap == ngay_thiet_lap,
        GiaMu.CongTy == gia_mu_update.CongTy
    ).first()
    try:
        if db_gia_mu:
            # Update existing record
            db_gia_mu.GiaMuNuoc = gia_mu_update.GiaMuNuoc
            db_gia_mu.GiaMuTap = gia_mu_update.GiaMuTap
        else:
            # Create new record
            db_gia_mu = GiaMu(
                NgayThietLap=ngay_thiet_lap,
                CongTy=gia_mu_update.CongTy,
                GiaMuNuoc=gia_mu_update.GiaMuNuoc,
                GiaMuTap=gia_mu_update.GiaMuTap
            )
            db.add(db_gia_mu)
        db.commit()
        db.refresh(db_gia_mu)
        logger.info(f"Updated GiaMu for date: {gia_mu_update.NgayThietLap}, CongTy: {gia_mu_update.CongTy}")
        return {
            "IDGiaMu": db_gia_mu.IDGiaMu,
            "NgayThietLap": db_gia_mu.NgayThietLap.strftime("%Y-%m-%d"),
            "CongTy": db_gia_mu.CongTy,
            "GiaMuNuoc": float(db_gia_mu.GiaMuNuoc),
            "GiaMuTap": float(db_gia_mu.GiaMuTap)
        }
    except Exception as e:
        db.rollback()
        logger.error(f"Unexpected error: {str(e)}")
        raise HTTPException(status_code=500, detail="Internal Server Error")

def get_gia_mu_by_date_and_company(db: Session, ngay_thiet_lap: date, cong_ty: str):
    logger.info(f"Fetching GiaMu for date: {ngay_thiet_lap}, CongTy: {cong_ty}")
    try:
        gia_mu = db.query(GiaMu).filter(
            GiaMu.NgayThietLap == ngay_thiet_lap,
            GiaMu.CongTy == cong_ty
        ).first()
        if not gia_mu:
            logger.warning(f"No GiaMu found for date {ngay_thiet_lap} and CongTy {cong_ty}")
            raise HTTPException(status_code=404, detail="No GiaMu found for specified date and company")
        return {
            "IDGiaMu": gia_mu.IDGiaMu,
            "NgayThietLap": gia_mu.NgayThietLap.strftime("%Y-%m-%d"),
            "CongTy": gia_mu.CongTy,
            "GiaMuNuoc": float(gia_mu.GiaMuNuoc),
            "GiaMuTap": float(gia_mu.GiaMuTap)
        }
    except Exception as e:
        logger.error(f"Unexpected error while fetching GiaMu: {str(e)}")
        raise HTTPException(status_code=500, detail="Internal Server Error")

def get_latest_gia_mu(db: Session, cong_ty: str):
    logger.info(f"Fetching latest GiaMu for CongTy: {cong_ty}")
    try:
        latest_gia_mu = db.query(GiaMu).filter(GiaMu.CongTy == cong_ty).order_by(GiaMu.NgayThietLap.desc(), GiaMu.IDGiaMu.desc()).first()
        if not latest_gia_mu:
            logger.warning(f"No GiaMu records found for CongTy {cong_ty}")
            raise HTTPException(status_code=404, detail=f"No GiaMu records found for CongTy {cong_ty}")
        return {
            "IDGiaMu": latest_gia_mu.IDGiaMu,
            "NgayThietLap": latest_gia_mu.NgayThietLap.strftime("%Y-%m-%d"),
            "CongTy": latest_gia_mu.CongTy,
            "GiaMuNuoc": float(latest_gia_mu.GiaMuNuoc),
            "GiaMuTap": float(latest_gia_mu.GiaMuTap)
        }
    except ValueError as e:
        logger.error(f"ValueError while converting GiaMu data: {str(e)}")
        raise HTTPException(status_code=500, detail=f"Error converting GiaMu data: {str(e)}")
    except Exception as e:
        logger.error(f"Unexpected error while fetching latest GiaMu: {str(e)}")
        raise HTTPException(status_code=500, detail="Internal Server Error")

def delete_gia_mu(db: Session, ngay_thiet_lap: date, cong_ty: str):
    logger.info(f"Deleting GiaMu for date: {ngay_thiet_lap}, CongTy: {cong_ty}")
    db_gia_mu = db.query(GiaMu).filter(
        GiaMu.NgayThietLap == ngay_thiet_lap,
        GiaMu.CongTy == cong_ty
    ).first()
    if not db_gia_mu:
        logger.warning(f"GiaMu for date {ngay_thiet_lap} and CongTy {cong_ty} not found")
        raise HTTPException(status_code=404, detail="GiaMu not found")

    try:
        db.delete(db_gia_mu)
        db.commit()
        logger.info(f"Deleted GiaMu for date: {ngay_thiet_lap}, CongTy: {cong_ty}")
        return {"detail": "GiaMu deleted"}
    except Exception as e:
        db.rollback()
        logger.error(f"Unexpected error: {str(e)}")
        raise HTTPException(status_code=500, detail="Internal Server Error")

# CRUD TriggerLog
def get_trigger_logs(db: Session, skip: int = 0, limit: int = 100):
    logger.info(f"Fetching TriggerLog records with skip={skip}, limit={limit}")
    try:
        result = db.query(TriggerLog).offset(skip).limit(limit).all()
        logger.info(f"Retrieved {len(result)} TriggerLog records")
        return result
    except Exception as e:
        logger.error(f"Unexpected error: {str(e)}")
        raise HTTPException(status_code=500, detail="Internal Server Error")

def update_trigger_log(db: Session, log_id: int, message: str):
    logger.info(f"Updating TriggerLog with LogID: {log_id}")
    db_trigger_log = db.query(TriggerLog).filter(TriggerLog.LogID == log_id).first()
    if not db_trigger_log:
        logger.warning(f"TriggerLog ID {log_id} not found")
        raise HTTPException(status_code=404, detail="TriggerLog not found")

    db_trigger_log.Message = message
    try:
        db.commit()
        db.refresh(db_trigger_log)
        logger.info(f"Updated TriggerLog with ID: {log_id}")
        return db_trigger_log
    except Exception as e:
        db.rollback()
        logger.error(f"Unexpected error: {str(e)}")
        raise HTTPException(status_code=500, detail="Internal Server Error")

def delete_trigger_log(db: Session, log_id: int):
    logger.info(f"Deleting TriggerLog with LogID: {log_id}")
    db_trigger_log = db.query(TriggerLog).filter(TriggerLog.LogID == log_id).first()
    if not db_trigger_log:
        logger.warning(f"TriggerLog ID {log_id} not found")
        raise HTTPException(status_code=404, detail="TriggerLog not found")

    try:
        db.delete(db_trigger_log)
        db.commit()
        logger.info(f"Deleted TriggerLog with ID: {log_id}")
        return {"detail": "TriggerLog deleted"}
    except Exception as e:
        db.rollback()
        logger.error(f"Unexpected error: {str(e)}")
        raise HTTPException(status_code=500, detail="Internal Server Error")

def delete_khach_hang(db: Session, taikhoan_id: int):
    logger.info(f"Deleting KhachHang with IDTaiKhoan: {taikhoan_id}")
    khach_hang = db.query(KhachHang).filter(KhachHang.IDTaiKhoan == taikhoan_id).first()
    if not khach_hang:
        logger.warning(f"KhachHang with IDTaiKhoan {taikhoan_id} not found")
        raise HTTPException(status_code=404, detail="KhachHang not found")

    # Xóa dữ liệu từ ThanhToan và GiaoDich dựa trên IDKhachHang
    db.query(ThanhToan).filter(ThanhToan.IDKhachHang == khach_hang.IDKhachHang).delete()
    db.query(GiaoDich).filter(GiaoDich.IDKhachHang == khach_hang.IDKhachHang).delete()

    # Xóa KhachHang
    db.query(KhachHang).filter(KhachHang.IDTaiKhoan == taikhoan_id).delete()

    # Xóa TaiKhoan
    db.query(TaiKhoan).filter(TaiKhoan.IDTaiKhoan == taikhoan_id).delete()

    try:
        db.commit()
        logger.info(f"Deleted KhachHang with IDTaiKhoan: {taikhoan_id}")
        return {"detail": "Deleted KhachHang successfully"}
    except Exception as e:
        db.rollback()
        logger.error(f"Unexpected error: {str(e)}")
        raise HTTPException(status_code=500, detail="Internal Server Error")

def get_khach_hang(db: Session, skip: int = 0, limit: int = 100, cong_ty: Optional[str] = None):
    logger.info(f"Fetching KhachHang with skip={skip}, limit={limit}, cong_ty={cong_ty}")
    query = db.query(KhachHang).join(TaiKhoan, KhachHang.IDTaiKhoan == TaiKhoan.IDTaiKhoan)
    if cong_ty:
        query = query.filter(KhachHang.CongTy == cong_ty)
    khach_hangs = query.offset(skip).limit(limit).all()
    result = []
    for kh in khach_hangs:
        if not kh.IDTaiKhoan or not kh.TaiKhoan:
            logger.warning(f"No valid TaiKhoan for KhachHang IDKhachHang={kh.IDKhachHang}, ten_dang_nhap={kh.ten_dang_nhap}")
            continue  # Bỏ qua nếu IDTaiKhoan là NULL hoặc không có TaiKhoan
        result.append({
            "IDKhachHang": kh.IDKhachHang,
            "ten_dang_nhap": kh.TaiKhoan.TenDangNhap,
            "HoVaTen": kh.HoVaTen,
            "Gmail": kh.Gmail,
            "RFID": kh.RFID,
            "SoDienThoai": kh.SoDienThoai,
            "CongTy": kh.CongTy,
            "SoTaiKhoan": kh.SoTaiKhoan,
            "NganHang": kh.NganHang,
            "IDTaiKhoan": kh.IDTaiKhoan
        })
    if not result:
        logger.warning(f"No valid KhachHang records found for CongTy {cong_ty}")
    return result

def delete_khach_hang_by_id_khachhang(db: Session, khachhang_id: int):
    logger.info(f"Deleting KhachHang with IDKhachHang: {khachhang_id}")
    khach_hang = db.query(KhachHang).filter(KhachHang.IDKhachHang == khachhang_id).first()
    if not khach_hang:
        logger.warning(f"KhachHang with IDKhachHang {khachhang_id} not found")
        raise HTTPException(status_code=404, detail="KhachHang not found")

    # Lấy SoDienThoai để tìm TaiKhoan
    so_dien_thoai = khach_hang.SoDienThoai
    if not so_dien_thoai:
        logger.warning(f"KhachHang with IDKhachHang {khachhang_id} has no SoDienThoai")
        raise HTTPException(status_code=400, detail="KhachHang has no SoDienThoai")

    # Tìm TaiKhoan với TenDangNhap khớp với SoDienThoai
    taikhoan = db.query(TaiKhoan).filter(TaiKhoan.TenDangNhap == so_dien_thoai).first()
    if not taikhoan:
        logger.warning(f"TaiKhoan with TenDangNhap {so_dien_thoai} not found")
        raise HTTPException(status_code=404, detail=f"TaiKhoan with TenDangNhap {so_dien_thoai} not found")

    try:
        # Xóa dữ liệu từ ThanhToan và GiaoDich dựa trên IDKhachHang
        db.query(ThanhToan).filter(ThanhToan.IDKhachHang == khach_hang.IDKhachHang).delete()
        db.query(GiaoDich).filter(GiaoDich.IDKhachHang == khach_hang.IDKhachHang).delete()

        # Xóa KhachHang
        db.query(KhachHang).filter(KhachHang.IDKhachHang == khachhang_id).delete()

        # Xóa TaiKhoan với TenDangNhap khớp với SoDienThoai
        db.query(TaiKhoan).filter(TaiKhoan.TenDangNhap == so_dien_thoai).delete()

        db.commit()
        logger.info(f"Deleted KhachHang with IDKhachHang: {khachhang_id} and TaiKhoan with TenDangNhap: {so_dien_thoai}")
        return {"detail": "Deleted KhachHang successfully"}
    except Exception as e:
        db.rollback()
        logger.error(f"Unexpected error: {str(e)}")
        raise HTTPException(status_code=500, detail="Internal Server Error")

# Add these functions to crud.py
# Replace the create_giaodich_mu_tap function
def create_giaodich_mu_tap(db: Session, giaodich: GiaoDichMuTapCreate):
    logger.info(f"Creating GiaoDich MuTap with RFID: {giaodich.RFID}")
    khach_hang = db.query(KhachHang).filter(KhachHang.RFID == giaodich.RFID).first()
    if not khach_hang:
        logger.warning(f"RFID {giaodich.RFID} not found")
        raise HTTPException(status_code=400, detail="RFID not found")

    current_time = datetime.utcnow()
    cong_ty = khach_hang.CongTy
    ngay_giao_dich = current_time.date()

    giao_dich_dict = {
        "IDKhachHang": khach_hang.IDKhachHang,
        "NgayGiaoDich": ngay_giao_dich,
        "ThoiGianGiaoDich": current_time.time(),
        "CongTy": cong_ty,
        "MuTap": giaodich.KhoiLuongMuTap,
        "DRC": 0.0,  # Will be updated later
        "MuNuoc": 0.0,  # Will be updated later
        "TSC": 0.0  # Will be updated later
    }
    db_giaodich = GiaoDich(**giao_dich_dict)
    try:
        db.add(db_giaodich)
        db.commit()
        db.refresh(db_giaodich)
        logger.info(f"Created GiaoDich MuTap with ID: {db_giaodich.IDGiaoDich}")
        return db_giaodich
    except SQLAlchemyError as e:
        db.rollback()
        error_msg = str(e)
        if "GiaoDich already exists" in error_msg:
            logger.warning(f"GiaoDich already exists for RFID {giaodich.RFID}")
            raise HTTPException(status_code=400, detail="GiaoDich already exists for this time")
        if "No previous GiaMu found" in error_msg:
            logger.warning(f"No previous GiaMu found for CongTy {cong_ty}")
            raise HTTPException(status_code=400, detail=f"No previous GiaMu found for CongTy {cong_ty} to create new price")
        logger.error(f"Unexpected error: {error_msg}")
        raise HTTPException(status_code=500, detail=f"Database error: {error_msg}")

def update_giaodich_mu_nuoc(db: Session, giaodich: GiaoDichMuNuocCreate):
    logger.info(f"Updating GiaoDich MuNuoc with RFID: {giaodich.RFID}")
    khach_hang = db.query(KhachHang).filter(KhachHang.RFID == giaodich.RFID).first()
    if not khach_hang:
        logger.warning(f"RFID {giaodich.RFID} not found")
        raise HTTPException(status_code=400, detail="RFID not found")

    # Tìm giao dịch mới nhất của khách hàng trong ngày hiện tại
    current_date = date.today()
    db_giaodich = db.query(GiaoDich).filter(
        GiaoDich.IDKhachHang == khach_hang.IDKhachHang,
        GiaoDich.NgayGiaoDich == current_date
    ).order_by(GiaoDich.ThoiGianGiaoDich.desc()).first()

    if not db_giaodich:
        logger.warning(f"No GiaoDich found for RFID {giaodich.RFID} on {current_date}")
        raise HTTPException(status_code=404, detail="No GiaoDich found for today")

    db_giaodich.MuNuoc = giaodich.KhoiLuongMuNuoc
    try:
        db.commit()
        db.refresh(db_giaodich)
        logger.info(f"Updated GiaoDich MuNuoc with ID: {db_giaodich.IDGiaoDich}")
        return db_giaodich
    except SQLAlchemyError as e:
        db.rollback()
        logger.error(f"Unexpected error: {str(e)}")
        raise HTTPException(status_code=500, detail="Internal Server Error")

def update_giaodich_tsc_drc(db: Session, giaodich: GiaoDichTSCDRCCreate):
    logger.info(f"Updating GiaoDich TSC/DRC with RFID: {giaodich.RFID}")
    khach_hang = db.query(KhachHang).filter(KhachHang.RFID == giaodich.RFID).first()
    if not khach_hang:
        logger.warning(f"RFID {giaodich.RFID} not found")
        raise HTTPException(status_code=400, detail="RFID not found")

    current_date = date.today()
    db_giaodich = db.query(GiaoDich).filter(
        GiaoDich.IDKhachHang == khach_hang.IDKhachHang,
        GiaoDich.NgayGiaoDich == current_date
    ).order_by(GiaoDich.ThoiGianGiaoDich.desc()).first()

    if not db_giaodich:
        logger.warning(f"No GiaoDich found for RFID {giaodich.RFID} on {current_date}")
        raise HTTPException(status_code=404, detail="No GiaoDich found for today")

    db_giaodich.TSC = giaodich.TSC
    db_giaodich.DRC = giaodich.DRC
    try:
        db.commit()
        db.refresh(db_giaodich)
        logger.info(f"Updated GiaoDich TSC/DRC with ID: {db_giaodich.IDGiaoDich}")
        return db_giaodich
    except SQLAlchemyError as e:
        db.rollback()
        logger.error(f"Unexpected error: {str(e)}")
        raise HTTPException(status_code=500, detail="Internal Server Error")