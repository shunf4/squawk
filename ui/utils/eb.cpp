/*
 * Squawk messenger. 
 * Copyright (C) 2019  Yury Gubich <blue@macaw.me>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "eb.h"

static const int tileSize = 32;
template <class T>
static
inline void qt_memrotate90_tiled(const T *src, int w, int h, int sstride, T *dest, int dstride)
{
    sstride /= sizeof(T);
    dstride /= sizeof(T);
    const int pack = sizeof(quint32) / sizeof(T);
    const int unaligned =
    qMin(uint((quintptr(dest) & (sizeof(quint32)-1)) / sizeof(T)), uint(h));
    const int restX = w % tileSize;
    const int restY = (h - unaligned) % tileSize;
    const int unoptimizedY = restY % pack;
    const int numTilesX = w / tileSize + (restX > 0);
    const int numTilesY = (h - unaligned) / tileSize + (restY >= pack);
    for (int tx = 0; tx < numTilesX; ++tx) {
        const int startx = w - tx * tileSize - 1;
        const int stopx = qMax(startx - tileSize, 0);
        if (unaligned) {
            for (int x = startx; x >= stopx; --x) {
                T *d = dest + (w - x - 1) * dstride;
                for (int y = 0; y < unaligned; ++y) {
                    *d++ = src[y * sstride + x];
                }
            }
        }
        for (int ty = 0; ty < numTilesY; ++ty) {
            const int starty = ty * tileSize + unaligned;
            const int stopy = qMin(starty + tileSize, h - unoptimizedY);
            for (int x = startx; x >= stopx; --x) {
                quint32 *d = reinterpret_cast<quint32*>(dest + (w - x - 1) * dstride + starty);
                for (int y = starty; y < stopy; y += pack) {
                    quint32 c = src[y * sstride + x];
                    for (int i = 1; i < pack; ++i) {
                        const int shift = (sizeof(T) * 8 * i);
                        const T color = src[(y + i) * sstride + x];
                        c |= color << shift;
                    }
                    *d++ = c;
                }
            }
        }
        if (unoptimizedY) {
            const int starty = h - unoptimizedY;
            for (int x = startx; x >= stopx; --x) {
                T *d = dest + (w - x - 1) * dstride + starty;
                for (int y = starty; y < h; ++y) {
                    *d++ = src[y * sstride + x];
                }
            }
        }
    }
}
template <class T>
static
inline void qt_memrotate90_tiled_unpacked(const T *src, int w, int h, int sstride, T *dest,
                                          int dstride)
{
    const int numTilesX = (w + tileSize - 1) / tileSize;
    const int numTilesY = (h + tileSize - 1) / tileSize;
    for (int tx = 0; tx < numTilesX; ++tx) {
        const int startx = w - tx * tileSize - 1;
        const int stopx = qMax(startx - tileSize, 0);
        for (int ty = 0; ty < numTilesY; ++ty) {
            const int starty = ty * tileSize;
            const int stopy = qMin(starty + tileSize, h);
            for (int x = startx; x >= stopx; --x) {
                T *d = (T *)((char*)dest + (w - x - 1) * dstride) + starty;
                const char *s = (const char*)(src + x) + starty * sstride;
                for (int y = starty; y < stopy; ++y) {
                    *d++ = *(const T *)(s);
                    s += sstride;
                }
            }
        }
    }
}
template <class T>
static
inline void qt_memrotate270_tiled(const T *src, int w, int h, int sstride, T *dest, int dstride)
{
    sstride /= sizeof(T);
    dstride /= sizeof(T);
    const int pack = sizeof(quint32) / sizeof(T);
    const int unaligned =
    qMin(uint((quintptr(dest) & (sizeof(quint32)-1)) / sizeof(T)), uint(h));
    const int restX = w % tileSize;
    const int restY = (h - unaligned) % tileSize;
    const int unoptimizedY = restY % pack;
    const int numTilesX = w / tileSize + (restX > 0);
    const int numTilesY = (h - unaligned) / tileSize + (restY >= pack);
    for (int tx = 0; tx < numTilesX; ++tx) {
        const int startx = tx * tileSize;
        const int stopx = qMin(startx + tileSize, w);
        if (unaligned) {
            for (int x = startx; x < stopx; ++x) {
                T *d = dest + x * dstride;
                for (int y = h - 1; y >= h - unaligned; --y) {
                    *d++ = src[y * sstride + x];
                }
            }
        }
        for (int ty = 0; ty < numTilesY; ++ty) {
            const int starty = h - 1 - unaligned - ty * tileSize;
            const int stopy = qMax(starty - tileSize, unoptimizedY);
            for (int x = startx; x < stopx; ++x) {
                quint32 *d = reinterpret_cast<quint32*>(dest + x * dstride
                + h - 1 - starty);
                for (int y = starty; y >= stopy; y -= pack) {
                    quint32 c = src[y * sstride + x];
                    for (int i = 1; i < pack; ++i) {
                        const int shift = (sizeof(T) * 8 * i);
                        const T color = src[(y - i) * sstride + x];
                        c |= color << shift;
                    }
                    *d++ = c;
                }
            }
        }
        if (unoptimizedY) {
            const int starty = unoptimizedY - 1;
            for (int x = startx; x < stopx; ++x) {
                T *d = dest + x * dstride + h - 1 - starty;
                for (int y = starty; y >= 0; --y) {
                    *d++ = src[y * sstride + x];
                }
            }
        }
    }
}
template <class T>
static
inline void qt_memrotate270_tiled_unpacked(const T *src, int w, int h, int sstride, T *dest,
                                           int dstride)
{
    const int numTilesX = (w + tileSize - 1) / tileSize;
    const int numTilesY = (h + tileSize - 1) / tileSize;
    for (int tx = 0; tx < numTilesX; ++tx) {
        const int startx = tx * tileSize;
        const int stopx = qMin(startx + tileSize, w);
        for (int ty = 0; ty < numTilesY; ++ty) {
            const int starty = h - 1 - ty * tileSize;
            const int stopy = qMax(starty - tileSize, 0);
            for (int x = startx; x < stopx; ++x) {
                T *d = (T*)((char*)dest + x * dstride) + h - 1 - starty;
                const char *s = (const char*)(src + x) + starty * sstride;
                for (int y = starty; y >= stopy; --y) {
                    *d++ = *(const T*)s;
                    s -= sstride;
                }
            }
        }
    }
}
template <class T>
static
inline void qt_memrotate90_template(const T *src, int srcWidth, int srcHeight, int srcStride,
                                    T *dest, int dstStride)
{
    #if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    // packed algorithm assumes little endian and that sizeof(quint32)/sizeof(T) is an integer
    if (sizeof(quint32) % sizeof(T) == 0)
        qt_memrotate90_tiled<T>(src, srcWidth, srcHeight, srcStride, dest, dstStride);
    else
        #endif
        qt_memrotate90_tiled_unpacked<T>(src, srcWidth, srcHeight, srcStride, dest, dstStride);
}
template <>
inline void qt_memrotate90_template<quint32>(const quint32 *src, int w, int h, int sstride, quint32 *dest, int dstride)
{
    // packed algorithm doesn't have any benefit for quint32
    qt_memrotate90_tiled_unpacked(src, w, h, sstride, dest, dstride);
}
template <>
inline void qt_memrotate90_template<quint64>(const quint64 *src, int w, int h, int sstride, quint64 *dest, int dstride)
{
    qt_memrotate90_tiled_unpacked(src, w, h, sstride, dest, dstride);
}
template <class T>
static
inline void qt_memrotate180_template(const T *src, int w, int h, int sstride, T *dest, int dstride)
{
    const char *s = (const char*)(src) + (h - 1) * sstride;
    for (int dy = 0; dy < h; ++dy) {
        T *d = reinterpret_cast<T*>((char *)(dest) + dy * dstride);
        src = reinterpret_cast<const T*>(s);
        for (int dx = 0; dx < w; ++dx) {
            d[dx] = src[w - 1 - dx];
        }
        s -= sstride;
    }
}
template <class T>
static
inline void qt_memrotate270_template(const T *src, int srcWidth, int srcHeight, int srcStride,
                                     T *dest, int dstStride)
{
    #if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    // packed algorithm assumes little endian and that sizeof(quint32)/sizeof(T) is an integer
    if (sizeof(quint32) % sizeof(T) == 0)
        qt_memrotate270_tiled<T>(src, srcWidth, srcHeight, srcStride, dest, dstStride);
    else
        #endif
        qt_memrotate270_tiled_unpacked<T>(src, srcWidth, srcHeight, srcStride, dest, dstStride);
}
template <>
inline void qt_memrotate270_template<quint32>(const quint32 *src, int w, int h, int sstride, quint32 *dest, int dstride)
{
    // packed algorithm doesn't have any benefit for quint32
    qt_memrotate270_tiled_unpacked(src, w, h, sstride, dest, dstride);
}
template <>
inline void qt_memrotate270_template<quint64>(const quint64 *src, int w, int h, int sstride, quint64 *dest, int dstride)
{
    qt_memrotate270_tiled_unpacked(src, w, h, sstride, dest, dstride);
}
#define QT_IMPL_MEMROTATE(type)                                     \
void qt_memrotate90(const type *src, int w, int h, int sstride, \
type *dest, int dstride)           \
{                                                                   \
    qt_memrotate90_template(src, w, h, sstride, dest, dstride);     \
}                                                                   \
void qt_memrotate180(const type *src, int w, int h, int sstride, \
type *dest, int dstride)          \
{                                                                   \
    qt_memrotate180_template(src, w, h, sstride, dest, dstride);    \
}                                                                   \
void qt_memrotate270(const type *src, int w, int h, int sstride, \
type *dest, int dstride)          \
{                                                                   \
    qt_memrotate270_template(src, w, h, sstride, dest, dstride);    \
}
#define QT_IMPL_SIMPLE_MEMROTATE(type)                              \
void qt_memrotate90(const type *src, int w, int h, int sstride,  \
type *dest, int dstride)           \
{                                                                   \
    qt_memrotate90_tiled_unpacked(src, w, h, sstride, dest, dstride); \
}                                                                   \
void qt_memrotate180(const type *src, int w, int h, int sstride, \
type *dest, int dstride)          \
{                                                                   \
    qt_memrotate180_template(src, w, h, sstride, dest, dstride);    \
}                                                                   \
void qt_memrotate270(const type *src, int w, int h, int sstride, \
type *dest, int dstride)          \
{                                                                   \
    qt_memrotate270_tiled_unpacked(src, w, h, sstride, dest, dstride); \
}
QT_IMPL_MEMROTATE(quint64)
QT_IMPL_MEMROTATE(quint32)
QT_IMPL_MEMROTATE(quint16)
QT_IMPL_MEMROTATE(quint8)
void qt_memrotate90_8(const uchar *srcPixels, int w, int h, int sbpl, uchar *destPixels, int dbpl)
{
    qt_memrotate90(srcPixels, w, h, sbpl, destPixels, dbpl);
}
void qt_memrotate180_8(const uchar *srcPixels, int w, int h, int sbpl, uchar *destPixels, int dbpl)
{
    qt_memrotate180(srcPixels, w, h, sbpl, destPixels, dbpl);
}
void qt_memrotate270_8(const uchar *srcPixels, int w, int h, int sbpl, uchar *destPixels, int dbpl)
{
    qt_memrotate270(srcPixels, w, h, sbpl, destPixels, dbpl);
}
void qt_memrotate90_16(const uchar *srcPixels, int w, int h, int sbpl, uchar *destPixels, int dbpl)
{
    qt_memrotate90((const ushort *)srcPixels, w, h, sbpl, (ushort *)destPixels, dbpl);
}
void qt_memrotate180_16(const uchar *srcPixels, int w, int h, int sbpl, uchar *destPixels, int dbpl)
{
    qt_memrotate180((const ushort *)srcPixels, w, h, sbpl, (ushort *)destPixels, dbpl);
}
void qt_memrotate270_16(const uchar *srcPixels, int w, int h, int sbpl, uchar *destPixels, int dbpl)
{
    qt_memrotate270((const ushort *)srcPixels, w, h, sbpl, (ushort *)destPixels, dbpl);
}
void qt_memrotate90_32(const uchar *srcPixels, int w, int h, int sbpl, uchar *destPixels, int dbpl)
{
    qt_memrotate90((const uint *)srcPixels, w, h, sbpl, (uint *)destPixels, dbpl);
}
void qt_memrotate180_32(const uchar *srcPixels, int w, int h, int sbpl, uchar *destPixels, int dbpl)
{
    qt_memrotate180((const uint *)srcPixels, w, h, sbpl, (uint *)destPixels, dbpl);
}
void qt_memrotate270_32(const uchar *srcPixels, int w, int h, int sbpl, uchar *destPixels, int dbpl)
{
    qt_memrotate270((const uint *)srcPixels, w, h, sbpl, (uint *)destPixels, dbpl);
}
void qt_memrotate90_64(const uchar *srcPixels, int w, int h, int sbpl, uchar *destPixels, int dbpl)
{
    qt_memrotate90((const quint64 *)srcPixels, w, h, sbpl, (quint64 *)destPixels, dbpl);
}
void qt_memrotate180_64(const uchar *srcPixels, int w, int h, int sbpl, uchar *destPixels, int dbpl)
{
    qt_memrotate180((const quint64 *)srcPixels, w, h, sbpl, (quint64 *)destPixels, dbpl);
}
void qt_memrotate270_64(const uchar *srcPixels, int w, int h, int sbpl, uchar *destPixels, int dbpl)
{
    qt_memrotate270((const quint64 *)srcPixels, w, h, sbpl, (quint64 *)destPixels, dbpl);
}

#define AVG(a,b)  ( ((((a)^(b)) & 0xfefefefeUL) >> 1) + ((a)&(b)) )
#define AVG16(a,b)  ( ((((a)^(b)) & 0xf7deUL) >> 1) + ((a)&(b)) )
const int alphaIndex = (QSysInfo::ByteOrder == QSysInfo::BigEndian ? 0 : 3);

QImage qt_halfScaled(const QImage &source)
{
    if (source.width() < 2 || source.height() < 2)
        return QImage();
    
    QImage srcImage = source;
    
    if (source.format() == QImage::Format_Indexed8 || source.format() == QImage::Format_Grayscale8) {
        // assumes grayscale
        QImage dest(source.width() / 2, source.height() / 2, srcImage.format());
        dest.setDevicePixelRatio(source.devicePixelRatioF());
        
        const uchar *src = reinterpret_cast<const uchar*>(const_cast<const QImage &>(srcImage).bits());
        qsizetype sx = srcImage.bytesPerLine();
        qsizetype sx2 = sx << 1;
        
        uchar *dst = reinterpret_cast<uchar*>(dest.bits());
        qsizetype dx = dest.bytesPerLine();
        int ww = dest.width();
        int hh = dest.height();
        
        for (int y = hh; y; --y, dst += dx, src += sx2) {
            const uchar *p1 = src;
            const uchar *p2 = src + sx;
            uchar *q = dst;
            for (int x = ww; x; --x, ++q, p1 += 2, p2 += 2)
                *q = ((int(p1[0]) + int(p1[1]) + int(p2[0]) + int(p2[1])) + 2) >> 2;
        }
        
        return dest;
    } else if (source.format() == QImage::Format_ARGB8565_Premultiplied) {
        QImage dest(source.width() / 2, source.height() / 2, srcImage.format());
        dest.setDevicePixelRatio(source.devicePixelRatioF());
        
        const uchar *src = reinterpret_cast<const uchar*>(const_cast<const QImage &>(srcImage).bits());
        qsizetype sx = srcImage.bytesPerLine();
        qsizetype sx2 = sx << 1;
        
        uchar *dst = reinterpret_cast<uchar*>(dest.bits());
        qsizetype dx = dest.bytesPerLine();
        int ww = dest.width();
        int hh = dest.height();
        
        for (int y = hh; y; --y, dst += dx, src += sx2) {
            const uchar *p1 = src;
            const uchar *p2 = src + sx;
            uchar *q = dst;
            for (int x = ww; x; --x, q += 3, p1 += 6, p2 += 6) {
                // alpha
                q[0] = AVG(AVG(p1[0], p1[3]), AVG(p2[0], p2[3]));
                // rgb
                const quint16 p16_1 = (p1[2] << 8) | p1[1];
                const quint16 p16_2 = (p1[5] << 8) | p1[4];
                const quint16 p16_3 = (p2[2] << 8) | p2[1];
                const quint16 p16_4 = (p2[5] << 8) | p2[4];
                const quint16 result = AVG16(AVG16(p16_1, p16_2), AVG16(p16_3, p16_4));
                q[1] = result & 0xff;
                q[2] = result >> 8;
            }
        }
        
        return dest;
    } else if (source.format() != QImage::Format_ARGB32_Premultiplied
        && source.format() != QImage::Format_RGB32)
    {
        srcImage = source.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    }
    
    QImage dest(source.width() / 2, source.height() / 2, srcImage.format());
    dest.setDevicePixelRatio(source.devicePixelRatioF());
    
    const quint32 *src = reinterpret_cast<const quint32*>(const_cast<const QImage &>(srcImage).bits());
    qsizetype sx = srcImage.bytesPerLine() >> 2;
    qsizetype sx2 = sx << 1;
    
    quint32 *dst = reinterpret_cast<quint32*>(dest.bits());
    qsizetype dx = dest.bytesPerLine() >> 2;
    int ww = dest.width();
    int hh = dest.height();
    
    for (int y = hh; y; --y, dst += dx, src += sx2) {
        const quint32 *p1 = src;
        const quint32 *p2 = src + sx;
        quint32 *q = dst;
        for (int x = ww; x; --x, q++, p1 += 2, p2 += 2)
            *q = AVG(AVG(p1[0], p1[1]), AVG(p2[0], p2[1]));
    }
    
    return dest;
}

template <int shift>
inline int qt_static_shift(int value)
{
    if (shift == 0)
        return value;
    else if (shift > 0)
        return value << (uint(shift) & 0x1f);
    else
        return value >> (uint(-shift) & 0x1f);
}

template<int aprec, int zprec>
inline void qt_blurinner(uchar *bptr, int &zR, int &zG, int &zB, int &zA, int alpha)
{
    QRgb *pixel = (QRgb *)bptr;
    
    #define Z_MASK (0xff << zprec)
    const int A_zprec = qt_static_shift<zprec - 24>(*pixel) & Z_MASK;
    const int R_zprec = qt_static_shift<zprec - 16>(*pixel) & Z_MASK;
    const int G_zprec = qt_static_shift<zprec - 8>(*pixel)  & Z_MASK;
    const int B_zprec = qt_static_shift<zprec>(*pixel)      & Z_MASK;
    #undef Z_MASK
    
    const int zR_zprec = zR >> aprec;
    const int zG_zprec = zG >> aprec;
    const int zB_zprec = zB >> aprec;
    const int zA_zprec = zA >> aprec;
    
    zR += alpha * (R_zprec - zR_zprec);
    zG += alpha * (G_zprec - zG_zprec);
    zB += alpha * (B_zprec - zB_zprec);
    zA += alpha * (A_zprec - zA_zprec);
    
    #define ZA_MASK (0xff << (zprec + aprec))
    *pixel =
    qt_static_shift<24 - zprec - aprec>(zA & ZA_MASK)
    | qt_static_shift<16 - zprec - aprec>(zR & ZA_MASK)
    | qt_static_shift<8 - zprec - aprec>(zG & ZA_MASK)
    | qt_static_shift<-zprec - aprec>(zB & ZA_MASK);
    #undef ZA_MASK
}

template<int aprec, int zprec>
inline void qt_blurinner_alphaOnly(uchar *bptr, int &z, int alpha)
{
    const int A_zprec = int(*(bptr)) << zprec;
    const int z_zprec = z >> aprec;
    z += alpha * (A_zprec - z_zprec);
    *(bptr) = z >> (zprec + aprec);
}

template<int aprec, int zprec, bool alphaOnly>
inline void qt_blurrow(QImage & im, int line, int alpha)
{
    uchar *bptr = im.scanLine(line);
    
    int zR = 0, zG = 0, zB = 0, zA = 0;
    
    if (alphaOnly && im.format() != QImage::Format_Indexed8)
        bptr += alphaIndex;
    
    const int stride = im.depth() >> 3;
    const int im_width = im.width();
    for (int index = 0; index < im_width; ++index) {
        if (alphaOnly)
            qt_blurinner_alphaOnly<aprec, zprec>(bptr, zA, alpha);
        else
            qt_blurinner<aprec, zprec>(bptr, zR, zG, zB, zA, alpha);
        bptr += stride;
    }
    
    bptr -= stride;
    
    for (int index = im_width - 2; index >= 0; --index) {
        bptr -= stride;
        if (alphaOnly)
            qt_blurinner_alphaOnly<aprec, zprec>(bptr, zA, alpha);
        else
            qt_blurinner<aprec, zprec>(bptr, zR, zG, zB, zA, alpha);
    }
}

template <int aprec, int zprec, bool alphaOnly>
void expblur(QImage &img, qreal radius, bool improvedQuality = false, int transposed = 0)
{
    // halve the radius if we're using two passes
    if (improvedQuality)
        radius *= qreal(0.5);
    
    Q_ASSERT(img.format() == QImage::Format_ARGB32_Premultiplied
    || img.format() == QImage::Format_RGB32
    || img.format() == QImage::Format_Indexed8
    || img.format() == QImage::Format_Grayscale8);
    
    // choose the alpha such that pixels at radius distance from a fully
    // saturated pixel will have an alpha component of no greater than
    // the cutOffIntensity
    const qreal cutOffIntensity = 2;
    int alpha = radius <= qreal(1e-5)
    ? ((1 << aprec)-1)
    : qRound((1<<aprec)*(1 - qPow(cutOffIntensity * (1 / qreal(255)), 1 / radius)));
    
    int img_height = img.height();
    for (int row = 0; row < img_height; ++row) {
        for (int i = 0; i <= int(improvedQuality); ++i)
            qt_blurrow<aprec, zprec, alphaOnly>(img, row, alpha);
    }
    
    QImage temp(img.height(), img.width(), img.format());
    temp.setDevicePixelRatio(img.devicePixelRatioF());
    if (transposed >= 0) {
        if (img.depth() == 8) {
            qt_memrotate270(reinterpret_cast<const quint8*>(img.bits()),
                            img.width(), img.height(), img.bytesPerLine(),
                            reinterpret_cast<quint8*>(temp.bits()),
                            temp.bytesPerLine());
        } else {
            qt_memrotate270(reinterpret_cast<const quint32*>(img.bits()),
                            img.width(), img.height(), img.bytesPerLine(),
                            reinterpret_cast<quint32*>(temp.bits()),
                            temp.bytesPerLine());
        }
    } else {
        if (img.depth() == 8) {
            qt_memrotate90(reinterpret_cast<const quint8*>(img.bits()),
                           img.width(), img.height(), img.bytesPerLine(),
                           reinterpret_cast<quint8*>(temp.bits()),
                           temp.bytesPerLine());
        } else {
            qt_memrotate90(reinterpret_cast<const quint32*>(img.bits()),
                           img.width(), img.height(), img.bytesPerLine(),
                           reinterpret_cast<quint32*>(temp.bits()),
                           temp.bytesPerLine());
        }
    }
    
    img_height = temp.height();
    for (int row = 0; row < img_height; ++row) {
        for (int i = 0; i <= int(improvedQuality); ++i)
            qt_blurrow<aprec, zprec, alphaOnly>(temp, row, alpha);
    }
    
    if (transposed == 0) {
        if (img.depth() == 8) {
            qt_memrotate90(reinterpret_cast<const quint8*>(temp.bits()),
                           temp.width(), temp.height(), temp.bytesPerLine(),
                           reinterpret_cast<quint8*>(img.bits()),
                           img.bytesPerLine());
        } else {
            qt_memrotate90(reinterpret_cast<const quint32*>(temp.bits()),
                           temp.width(), temp.height(), temp.bytesPerLine(),
                           reinterpret_cast<quint32*>(img.bits()),
                           img.bytesPerLine());
        }
    } else {
        img = temp;
    }
}

void Utils::exponentialblur(QImage& img, qreal radius, bool improvedQuality, int transposed)
{
    expblur<12, 10, false>(img, radius, improvedQuality, transposed);
}
