

#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>
#include <utility>
#include <cstddef>
#include <limits>
#include <vector>
#include <cstdint>

#include "fft_ops.h"



#include <mutex>  // NOLINT
#include <sstream>
#include <string>

#include <cmath>  // logf, sqrtf, cosf
#include <cstdint>
#include <cstdlib>  // RAND_MAX

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

#ifndef M_2PI
#define M_2PI 6.283185307179586476925286766559005
#endif

#ifndef M_SQRT2
#define M_SQRT2 1.4142135623730950488016887
#endif

namespace knf {

    inline float Log(float x) { return logf(x); }

    // Returns a random integer between 0 and RAND_MAX, inclusive
    int Rand(struct RandomState* state = NULL);

    // State for thread-safe random number generator
    struct RandomState {
        RandomState();
        unsigned seed;
    };

    /// Returns a random number strictly between 0 and 1.
    inline float RandUniform(struct RandomState* state = NULL) {
        return static_cast<float>((Rand(state) + 1.0) / (RAND_MAX + 2.0));
    }

    inline float RandGauss(struct RandomState* state = NULL) {
        return static_cast<float>(sqrtf(-2 * Log(RandUniform(state))) *
            cosf(2 * M_PI * RandUniform(state)));
    }

    void Sqrt(float* in_out, int32_t n);


    int Rand(struct RandomState* state) {
#if !defined(_POSIX_THREAD_SAFE_FUNCTIONS)
        // On Windows and Cygwin, just call Rand()
        return rand();
#else
        if (state) {
            return rand_r(&(state->seed));
        }
        else {
            static std::mutex _RandMutex;
            std::lock_guard<std::mutex> lock(_RandMutex);
            return rand();
        }
#endif
    }

    RandomState::RandomState() {
        // we initialize it as Rand() + 27437 instead of just Rand(), because on some
        // systems, e.g. at the very least Mac OSX Yosemite and later, it seems to be
        // the case that rand_r when initialized with rand() will give you the exact
        // same sequence of numbers that rand() will give if you keep calling rand()
        // after that initial call.  This can cause problems with repeated sequences.
        // For example if you initialize two RandomState structs one after the other
        // without calling rand() in between, they would give you the same sequence
        // offset by one (if we didn't have the "+ 27437" in the code).  27437 is just
        // a randomly chosen prime number.
        seed = unsigned(Rand()) + 27437;
    }

    void Sqrt(float* in_out, int32_t n) {
        for (int32_t i = 0; i != n; ++i) {
            in_out[i] = std::sqrt(in_out[i]);
        }
    }



    class Voidifier {
    public:
#if KNF_ENABLE_CHECK
        void operator&(const Logger&) const {}
#endif
    };
#if !defined(KNF_ENABLE_CHECK)
    template <typename T>
    const Voidifier& operator<<(const Voidifier& v, T&&) {
        return v;
    }
#endif

}  // namespace knf

#define KNF_STATIC_ASSERT(x) static_assert(x, "")
#define KNF_CHECK(x) ::knf::Voidifier()
#define KNF_LOG(x) ::knf::Voidifier()

#define KNF_CHECK_EQ(x, y) ::knf::Voidifier()
#define KNF_CHECK_NE(x, y) ::knf::Voidifier()
#define KNF_CHECK_LT(x, y) ::knf::Voidifier()
#define KNF_CHECK_LE(x, y) ::knf::Voidifier()
#define KNF_CHECK_GT(x, y) ::knf::Voidifier()
#define KNF_CHECK_GE(x, y) ::knf::Voidifier()

#define KNF_DCHECK(x) ::knf::Voidifier()
#define KNF_DLOG(x) ::knf::Voidifier()
#define KNF_DCHECK_EQ(x, y) ::knf::Voidifier()
#define KNF_DCHECK_NE(x, y) ::knf::Voidifier()
#define KNF_DCHECK_LT(x, y) ::knf::Voidifier()
#define KNF_DCHECK_LE(x, y) ::knf::Voidifier()
#define KNF_DCHECK_GT(x, y) ::knf::Voidifier()
#define KNF_DCHECK_GE(x, y) ::knf::Voidifier()


namespace knf {

    // see fftsg.cc
    void rdft(int n, int isgn, double* a, int* ip, double* w);

    class Rfft::RfftImpl {
    public:
        RfftImpl(int32_t n, bool inverse)
            : n_(n), inverse_(inverse), ip_(2 + std::sqrt(n / 2)), w_(n / 2) {
            if ((n & (n - 1)) != 0) {
                fprintf(stderr,
                    "Please set round_to_power_of_two to true. Note that it is ok "
                    "even if your trained model uses round_to_power_of_two=false\n");
                exit(-1);
            }
        }

        void Compute(float* in_out) {
            std::vector<double> d(in_out, in_out + n_);

            Compute(d.data());

            std::copy(d.begin(), d.end(), in_out);
        }

        void Compute(double* in_out) {
            // 1 means forward fft
            rdft(n_, inverse_ ? -1 : 1, in_out, ip_.data(), w_.data());
        }

    private:
        int32_t n_;
        bool inverse_ = false;
        std::vector<int32_t> ip_;
        std::vector<double> w_;
    };

    Rfft::Rfft(int32_t n, bool inverse /*=false*/)
        : impl_(std::make_unique<RfftImpl>(n, inverse)) {
    }

    Rfft::~Rfft() = default;

    void Rfft::Compute(float* in_out) { impl_->Compute(in_out); }
    void Rfft::Compute(double* in_out) { impl_->Compute(in_out); }


    static void makeipt(int nw, int* ip)
    {
        int j, l, m, m2, p, q;

        ip[2] = 0;
        ip[3] = 16;
        m = 2;
        for (l = nw; l > 32; l >>= 2) {
            m2 = m << 1;
            q = m2 << 3;
            for (j = m; j < m2; j++) {
                p = ip[j] << 2;
                ip[m + j] = p;
                ip[m2 + j] = p + q;
            }
            m = m2;
        }
    }

    static void makewt(int nw, int* ip, double* w)
    {
        int j, nwh, nw0, nw1;
        double delta, wn4r, wk1r, wk1i, wk3r, wk3i;

        ip[0] = nw;
        ip[1] = 1;
        if (nw > 2) {
            nwh = nw >> 1;
            delta = atan(1.0) / nwh;
            wn4r = cos(delta * nwh);
            w[0] = 1;
            w[1] = wn4r;
            if (nwh == 4) {
                w[2] = cos(delta * 2);
                w[3] = sin(delta * 2);
            }
            else if (nwh > 4) {
                makeipt(nw, ip);
                w[2] = 0.5 / cos(delta * 2);
                w[3] = 0.5 / cos(delta * 6);
                for (j = 4; j < nwh; j += 4) {
                    w[j] = cos(delta * j);
                    w[j + 1] = sin(delta * j);
                    w[j + 2] = cos(3 * delta * j);
                    w[j + 3] = -sin(3 * delta * j);
                }
            }
            nw0 = 0;
            while (nwh > 2) {
                nw1 = nw0 + nwh;
                nwh >>= 1;
                w[nw1] = 1;
                w[nw1 + 1] = wn4r;
                if (nwh == 4) {
                    wk1r = w[nw0 + 4];
                    wk1i = w[nw0 + 5];
                    w[nw1 + 2] = wk1r;
                    w[nw1 + 3] = wk1i;
                }
                else if (nwh > 4) {
                    wk1r = w[nw0 + 4];
                    wk3r = w[nw0 + 6];
                    w[nw1 + 2] = 0.5 / wk1r;
                    w[nw1 + 3] = 0.5 / wk3r;
                    for (j = 4; j < nwh; j += 4) {
                        wk1r = w[nw0 + 2 * j];
                        wk1i = w[nw0 + 2 * j + 1];
                        wk3r = w[nw0 + 2 * j + 2];
                        wk3i = w[nw0 + 2 * j + 3];
                        w[nw1 + j] = wk1r;
                        w[nw1 + j + 1] = wk1i;
                        w[nw1 + j + 2] = wk3r;
                        w[nw1 + j + 3] = wk3i;
                    }
                }
                nw0 = nw1;
            }
        }
    }

    static void makect(int nc, int* ip, double* c)
    {
        int j, nch;
        double delta;

        ip[1] = nc;
        if (nc > 1) {
            nch = nc >> 1;
            delta = atan(1.0) / nch;
            c[0] = cos(delta * nch);
            c[nch] = 0.5 * c[0];
            for (j = 1; j < nch; j++) {
                c[j] = 0.5 * cos(delta * j);
                c[nc - j] = 0.5 * sin(delta * j);
            }
        }
    }

    /* -------- child routines -------- */

    static void bitrv2(int n, int* ip, double* a)
    {
        int j, j1, k, k1, l, m, nh, nm;
        double xr, xi, yr, yi;

        m = 1;
        for (l = n >> 2; l > 8; l >>= 2) {
            m <<= 1;
        }
        nh = n >> 1;
        nm = 4 * m;
        if (l == 8) {
            for (k = 0; k < m; k++) {
                for (j = 0; j < k; j++) {
                    j1 = 4 * j + 2 * ip[m + k];
                    k1 = 4 * k + 2 * ip[m + j];
                    xr = a[j1];
                    xi = a[j1 + 1];
                    yr = a[k1];
                    yi = a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 += nm;
                    k1 += 2 * nm;
                    xr = a[j1];
                    xi = a[j1 + 1];
                    yr = a[k1];
                    yi = a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 += nm;
                    k1 -= nm;
                    xr = a[j1];
                    xi = a[j1 + 1];
                    yr = a[k1];
                    yi = a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 += nm;
                    k1 += 2 * nm;
                    xr = a[j1];
                    xi = a[j1 + 1];
                    yr = a[k1];
                    yi = a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 += nh;
                    k1 += 2;
                    xr = a[j1];
                    xi = a[j1 + 1];
                    yr = a[k1];
                    yi = a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 -= nm;
                    k1 -= 2 * nm;
                    xr = a[j1];
                    xi = a[j1 + 1];
                    yr = a[k1];
                    yi = a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 -= nm;
                    k1 += nm;
                    xr = a[j1];
                    xi = a[j1 + 1];
                    yr = a[k1];
                    yi = a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 -= nm;
                    k1 -= 2 * nm;
                    xr = a[j1];
                    xi = a[j1 + 1];
                    yr = a[k1];
                    yi = a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 += 2;
                    k1 += nh;
                    xr = a[j1];
                    xi = a[j1 + 1];
                    yr = a[k1];
                    yi = a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 += nm;
                    k1 += 2 * nm;
                    xr = a[j1];
                    xi = a[j1 + 1];
                    yr = a[k1];
                    yi = a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 += nm;
                    k1 -= nm;
                    xr = a[j1];
                    xi = a[j1 + 1];
                    yr = a[k1];
                    yi = a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 += nm;
                    k1 += 2 * nm;
                    xr = a[j1];
                    xi = a[j1 + 1];
                    yr = a[k1];
                    yi = a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 -= nh;
                    k1 -= 2;
                    xr = a[j1];
                    xi = a[j1 + 1];
                    yr = a[k1];
                    yi = a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 -= nm;
                    k1 -= 2 * nm;
                    xr = a[j1];
                    xi = a[j1 + 1];
                    yr = a[k1];
                    yi = a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 -= nm;
                    k1 += nm;
                    xr = a[j1];
                    xi = a[j1 + 1];
                    yr = a[k1];
                    yi = a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 -= nm;
                    k1 -= 2 * nm;
                    xr = a[j1];
                    xi = a[j1 + 1];
                    yr = a[k1];
                    yi = a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                }
                k1 = 4 * k + 2 * ip[m + k];
                j1 = k1 + 2;
                k1 += nh;
                xr = a[j1];
                xi = a[j1 + 1];
                yr = a[k1];
                yi = a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
                j1 += nm;
                k1 += 2 * nm;
                xr = a[j1];
                xi = a[j1 + 1];
                yr = a[k1];
                yi = a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
                j1 += nm;
                k1 -= nm;
                xr = a[j1];
                xi = a[j1 + 1];
                yr = a[k1];
                yi = a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
                j1 -= 2;
                k1 -= nh;
                xr = a[j1];
                xi = a[j1 + 1];
                yr = a[k1];
                yi = a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
                j1 += nh + 2;
                k1 += nh + 2;
                xr = a[j1];
                xi = a[j1 + 1];
                yr = a[k1];
                yi = a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
                j1 -= nh - nm;
                k1 += 2 * nm - 2;
                xr = a[j1];
                xi = a[j1 + 1];
                yr = a[k1];
                yi = a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
            }
        }
        else {
            for (k = 0; k < m; k++) {
                for (j = 0; j < k; j++) {
                    j1 = 4 * j + ip[m + k];
                    k1 = 4 * k + ip[m + j];
                    xr = a[j1];
                    xi = a[j1 + 1];
                    yr = a[k1];
                    yi = a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 += nm;
                    k1 += nm;
                    xr = a[j1];
                    xi = a[j1 + 1];
                    yr = a[k1];
                    yi = a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 += nh;
                    k1 += 2;
                    xr = a[j1];
                    xi = a[j1 + 1];
                    yr = a[k1];
                    yi = a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 -= nm;
                    k1 -= nm;
                    xr = a[j1];
                    xi = a[j1 + 1];
                    yr = a[k1];
                    yi = a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 += 2;
                    k1 += nh;
                    xr = a[j1];
                    xi = a[j1 + 1];
                    yr = a[k1];
                    yi = a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 += nm;
                    k1 += nm;
                    xr = a[j1];
                    xi = a[j1 + 1];
                    yr = a[k1];
                    yi = a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 -= nh;
                    k1 -= 2;
                    xr = a[j1];
                    xi = a[j1 + 1];
                    yr = a[k1];
                    yi = a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 -= nm;
                    k1 -= nm;
                    xr = a[j1];
                    xi = a[j1 + 1];
                    yr = a[k1];
                    yi = a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                }
                k1 = 4 * k + ip[m + k];
                j1 = k1 + 2;
                k1 += nh;
                xr = a[j1];
                xi = a[j1 + 1];
                yr = a[k1];
                yi = a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
                j1 += nm;
                k1 += nm;
                xr = a[j1];
                xi = a[j1 + 1];
                yr = a[k1];
                yi = a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
            }
        }
    }

    static void bitrv2conj(int n, int* ip, double* a)
    {
        int j, j1, k, k1, l, m, nh, nm;
        double xr, xi, yr, yi;

        m = 1;
        for (l = n >> 2; l > 8; l >>= 2) {
            m <<= 1;
        }
        nh = n >> 1;
        nm = 4 * m;
        if (l == 8) {
            for (k = 0; k < m; k++) {
                for (j = 0; j < k; j++) {
                    j1 = 4 * j + 2 * ip[m + k];
                    k1 = 4 * k + 2 * ip[m + j];
                    xr = a[j1];
                    xi = -a[j1 + 1];
                    yr = a[k1];
                    yi = -a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 += nm;
                    k1 += 2 * nm;
                    xr = a[j1];
                    xi = -a[j1 + 1];
                    yr = a[k1];
                    yi = -a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 += nm;
                    k1 -= nm;
                    xr = a[j1];
                    xi = -a[j1 + 1];
                    yr = a[k1];
                    yi = -a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 += nm;
                    k1 += 2 * nm;
                    xr = a[j1];
                    xi = -a[j1 + 1];
                    yr = a[k1];
                    yi = -a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 += nh;
                    k1 += 2;
                    xr = a[j1];
                    xi = -a[j1 + 1];
                    yr = a[k1];
                    yi = -a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 -= nm;
                    k1 -= 2 * nm;
                    xr = a[j1];
                    xi = -a[j1 + 1];
                    yr = a[k1];
                    yi = -a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 -= nm;
                    k1 += nm;
                    xr = a[j1];
                    xi = -a[j1 + 1];
                    yr = a[k1];
                    yi = -a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 -= nm;
                    k1 -= 2 * nm;
                    xr = a[j1];
                    xi = -a[j1 + 1];
                    yr = a[k1];
                    yi = -a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 += 2;
                    k1 += nh;
                    xr = a[j1];
                    xi = -a[j1 + 1];
                    yr = a[k1];
                    yi = -a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 += nm;
                    k1 += 2 * nm;
                    xr = a[j1];
                    xi = -a[j1 + 1];
                    yr = a[k1];
                    yi = -a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 += nm;
                    k1 -= nm;
                    xr = a[j1];
                    xi = -a[j1 + 1];
                    yr = a[k1];
                    yi = -a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 += nm;
                    k1 += 2 * nm;
                    xr = a[j1];
                    xi = -a[j1 + 1];
                    yr = a[k1];
                    yi = -a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 -= nh;
                    k1 -= 2;
                    xr = a[j1];
                    xi = -a[j1 + 1];
                    yr = a[k1];
                    yi = -a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 -= nm;
                    k1 -= 2 * nm;
                    xr = a[j1];
                    xi = -a[j1 + 1];
                    yr = a[k1];
                    yi = -a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 -= nm;
                    k1 += nm;
                    xr = a[j1];
                    xi = -a[j1 + 1];
                    yr = a[k1];
                    yi = -a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 -= nm;
                    k1 -= 2 * nm;
                    xr = a[j1];
                    xi = -a[j1 + 1];
                    yr = a[k1];
                    yi = -a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                }
                k1 = 4 * k + 2 * ip[m + k];
                j1 = k1 + 2;
                k1 += nh;
                a[j1 - 1] = -a[j1 - 1];
                xr = a[j1];
                xi = -a[j1 + 1];
                yr = a[k1];
                yi = -a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
                a[k1 + 3] = -a[k1 + 3];
                j1 += nm;
                k1 += 2 * nm;
                xr = a[j1];
                xi = -a[j1 + 1];
                yr = a[k1];
                yi = -a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
                j1 += nm;
                k1 -= nm;
                xr = a[j1];
                xi = -a[j1 + 1];
                yr = a[k1];
                yi = -a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
                j1 -= 2;
                k1 -= nh;
                xr = a[j1];
                xi = -a[j1 + 1];
                yr = a[k1];
                yi = -a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
                j1 += nh + 2;
                k1 += nh + 2;
                xr = a[j1];
                xi = -a[j1 + 1];
                yr = a[k1];
                yi = -a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
                j1 -= nh - nm;
                k1 += 2 * nm - 2;
                a[j1 - 1] = -a[j1 - 1];
                xr = a[j1];
                xi = -a[j1 + 1];
                yr = a[k1];
                yi = -a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
                a[k1 + 3] = -a[k1 + 3];
            }
        }
        else {
            for (k = 0; k < m; k++) {
                for (j = 0; j < k; j++) {
                    j1 = 4 * j + ip[m + k];
                    k1 = 4 * k + ip[m + j];
                    xr = a[j1];
                    xi = -a[j1 + 1];
                    yr = a[k1];
                    yi = -a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 += nm;
                    k1 += nm;
                    xr = a[j1];
                    xi = -a[j1 + 1];
                    yr = a[k1];
                    yi = -a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 += nh;
                    k1 += 2;
                    xr = a[j1];
                    xi = -a[j1 + 1];
                    yr = a[k1];
                    yi = -a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 -= nm;
                    k1 -= nm;
                    xr = a[j1];
                    xi = -a[j1 + 1];
                    yr = a[k1];
                    yi = -a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 += 2;
                    k1 += nh;
                    xr = a[j1];
                    xi = -a[j1 + 1];
                    yr = a[k1];
                    yi = -a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 += nm;
                    k1 += nm;
                    xr = a[j1];
                    xi = -a[j1 + 1];
                    yr = a[k1];
                    yi = -a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 -= nh;
                    k1 -= 2;
                    xr = a[j1];
                    xi = -a[j1 + 1];
                    yr = a[k1];
                    yi = -a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                    j1 -= nm;
                    k1 -= nm;
                    xr = a[j1];
                    xi = -a[j1 + 1];
                    yr = a[k1];
                    yi = -a[k1 + 1];
                    a[j1] = yr;
                    a[j1 + 1] = yi;
                    a[k1] = xr;
                    a[k1 + 1] = xi;
                }
                k1 = 4 * k + ip[m + k];
                j1 = k1 + 2;
                k1 += nh;
                a[j1 - 1] = -a[j1 - 1];
                xr = a[j1];
                xi = -a[j1 + 1];
                yr = a[k1];
                yi = -a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
                a[k1 + 3] = -a[k1 + 3];
                j1 += nm;
                k1 += nm;
                a[j1 - 1] = -a[j1 - 1];
                xr = a[j1];
                xi = -a[j1 + 1];
                yr = a[k1];
                yi = -a[k1 + 1];
                a[j1] = yr;
                a[j1 + 1] = yi;
                a[k1] = xr;
                a[k1 + 1] = xi;
                a[k1 + 3] = -a[k1 + 3];
            }
        }
    }

    static void bitrv216(double* a)
    {
        double x1r, x1i, x2r, x2i, x3r, x3i, x4r, x4i,
            x5r, x5i, x7r, x7i, x8r, x8i, x10r, x10i,
            x11r, x11i, x12r, x12i, x13r, x13i, x14r, x14i;

        x1r = a[2];
        x1i = a[3];
        x2r = a[4];
        x2i = a[5];
        x3r = a[6];
        x3i = a[7];
        x4r = a[8];
        x4i = a[9];
        x5r = a[10];
        x5i = a[11];
        x7r = a[14];
        x7i = a[15];
        x8r = a[16];
        x8i = a[17];
        x10r = a[20];
        x10i = a[21];
        x11r = a[22];
        x11i = a[23];
        x12r = a[24];
        x12i = a[25];
        x13r = a[26];
        x13i = a[27];
        x14r = a[28];
        x14i = a[29];
        a[2] = x8r;
        a[3] = x8i;
        a[4] = x4r;
        a[5] = x4i;
        a[6] = x12r;
        a[7] = x12i;
        a[8] = x2r;
        a[9] = x2i;
        a[10] = x10r;
        a[11] = x10i;
        a[14] = x14r;
        a[15] = x14i;
        a[16] = x1r;
        a[17] = x1i;
        a[20] = x5r;
        a[21] = x5i;
        a[22] = x13r;
        a[23] = x13i;
        a[24] = x3r;
        a[25] = x3i;
        a[26] = x11r;
        a[27] = x11i;
        a[28] = x7r;
        a[29] = x7i;
    }

    static void bitrv216neg(double* a)
    {
        double x1r, x1i, x2r, x2i, x3r, x3i, x4r, x4i,
            x5r, x5i, x6r, x6i, x7r, x7i, x8r, x8i,
            x9r, x9i, x10r, x10i, x11r, x11i, x12r, x12i,
            x13r, x13i, x14r, x14i, x15r, x15i;

        x1r = a[2];
        x1i = a[3];
        x2r = a[4];
        x2i = a[5];
        x3r = a[6];
        x3i = a[7];
        x4r = a[8];
        x4i = a[9];
        x5r = a[10];
        x5i = a[11];
        x6r = a[12];
        x6i = a[13];
        x7r = a[14];
        x7i = a[15];
        x8r = a[16];
        x8i = a[17];
        x9r = a[18];
        x9i = a[19];
        x10r = a[20];
        x10i = a[21];
        x11r = a[22];
        x11i = a[23];
        x12r = a[24];
        x12i = a[25];
        x13r = a[26];
        x13i = a[27];
        x14r = a[28];
        x14i = a[29];
        x15r = a[30];
        x15i = a[31];
        a[2] = x15r;
        a[3] = x15i;
        a[4] = x7r;
        a[5] = x7i;
        a[6] = x11r;
        a[7] = x11i;
        a[8] = x3r;
        a[9] = x3i;
        a[10] = x13r;
        a[11] = x13i;
        a[12] = x5r;
        a[13] = x5i;
        a[14] = x9r;
        a[15] = x9i;
        a[16] = x1r;
        a[17] = x1i;
        a[18] = x14r;
        a[19] = x14i;
        a[20] = x6r;
        a[21] = x6i;
        a[22] = x10r;
        a[23] = x10i;
        a[24] = x2r;
        a[25] = x2i;
        a[26] = x12r;
        a[27] = x12i;
        a[28] = x4r;
        a[29] = x4i;
        a[30] = x8r;
        a[31] = x8i;
    }

    static void bitrv208(double* a)
    {
        double x1r, x1i, x3r, x3i, x4r, x4i, x6r, x6i;

        x1r = a[2];
        x1i = a[3];
        x3r = a[6];
        x3i = a[7];
        x4r = a[8];
        x4i = a[9];
        x6r = a[12];
        x6i = a[13];
        a[2] = x4r;
        a[3] = x4i;
        a[6] = x6r;
        a[7] = x6i;
        a[8] = x1r;
        a[9] = x1i;
        a[12] = x3r;
        a[13] = x3i;
    }

    static void bitrv208neg(double* a)
    {
        double x1r, x1i, x2r, x2i, x3r, x3i, x4r, x4i,
            x5r, x5i, x6r, x6i, x7r, x7i;

        x1r = a[2];
        x1i = a[3];
        x2r = a[4];
        x2i = a[5];
        x3r = a[6];
        x3i = a[7];
        x4r = a[8];
        x4i = a[9];
        x5r = a[10];
        x5i = a[11];
        x6r = a[12];
        x6i = a[13];
        x7r = a[14];
        x7i = a[15];
        a[2] = x7r;
        a[3] = x7i;
        a[4] = x3r;
        a[5] = x3i;
        a[6] = x5r;
        a[7] = x5i;
        a[8] = x1r;
        a[9] = x1i;
        a[10] = x6r;
        a[11] = x6i;
        a[12] = x2r;
        a[13] = x2i;
        a[14] = x4r;
        a[15] = x4i;
    }

    static void cftf1st(int n, double* a, double* w)
    {
        int j, j0, j1, j2, j3, k, m, mh;
        double wn4r, csc1, csc3, wk1r, wk1i, wk3r, wk3i,
            wd1r, wd1i, wd3r, wd3i;
        double x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i,
            y0r, y0i, y1r, y1i, y2r, y2i, y3r, y3i;

        mh = n >> 3;
        m = 2 * mh;
        j1 = m;
        j2 = j1 + m;
        j3 = j2 + m;
        x0r = a[0] + a[j2];
        x0i = a[1] + a[j2 + 1];
        x1r = a[0] - a[j2];
        x1i = a[1] - a[j2 + 1];
        x2r = a[j1] + a[j3];
        x2i = a[j1 + 1] + a[j3 + 1];
        x3r = a[j1] - a[j3];
        x3i = a[j1 + 1] - a[j3 + 1];
        a[0] = x0r + x2r;
        a[1] = x0i + x2i;
        a[j1] = x0r - x2r;
        a[j1 + 1] = x0i - x2i;
        a[j2] = x1r - x3i;
        a[j2 + 1] = x1i + x3r;
        a[j3] = x1r + x3i;
        a[j3 + 1] = x1i - x3r;
        wn4r = w[1];
        csc1 = w[2];
        csc3 = w[3];
        wd1r = 1;
        wd1i = 0;
        wd3r = 1;
        wd3i = 0;
        k = 0;
        for (j = 2; j < mh - 2; j += 4) {
            k += 4;
            wk1r = csc1 * (wd1r + w[k]);
            wk1i = csc1 * (wd1i + w[k + 1]);
            wk3r = csc3 * (wd3r + w[k + 2]);
            wk3i = csc3 * (wd3i + w[k + 3]);
            wd1r = w[k];
            wd1i = w[k + 1];
            wd3r = w[k + 2];
            wd3i = w[k + 3];
            j1 = j + m;
            j2 = j1 + m;
            j3 = j2 + m;
            x0r = a[j] + a[j2];
            x0i = a[j + 1] + a[j2 + 1];
            x1r = a[j] - a[j2];
            x1i = a[j + 1] - a[j2 + 1];
            y0r = a[j + 2] + a[j2 + 2];
            y0i = a[j + 3] + a[j2 + 3];
            y1r = a[j + 2] - a[j2 + 2];
            y1i = a[j + 3] - a[j2 + 3];
            x2r = a[j1] + a[j3];
            x2i = a[j1 + 1] + a[j3 + 1];
            x3r = a[j1] - a[j3];
            x3i = a[j1 + 1] - a[j3 + 1];
            y2r = a[j1 + 2] + a[j3 + 2];
            y2i = a[j1 + 3] + a[j3 + 3];
            y3r = a[j1 + 2] - a[j3 + 2];
            y3i = a[j1 + 3] - a[j3 + 3];
            a[j] = x0r + x2r;
            a[j + 1] = x0i + x2i;
            a[j + 2] = y0r + y2r;
            a[j + 3] = y0i + y2i;
            a[j1] = x0r - x2r;
            a[j1 + 1] = x0i - x2i;
            a[j1 + 2] = y0r - y2r;
            a[j1 + 3] = y0i - y2i;
            x0r = x1r - x3i;
            x0i = x1i + x3r;
            a[j2] = wk1r * x0r - wk1i * x0i;
            a[j2 + 1] = wk1r * x0i + wk1i * x0r;
            x0r = y1r - y3i;
            x0i = y1i + y3r;
            a[j2 + 2] = wd1r * x0r - wd1i * x0i;
            a[j2 + 3] = wd1r * x0i + wd1i * x0r;
            x0r = x1r + x3i;
            x0i = x1i - x3r;
            a[j3] = wk3r * x0r + wk3i * x0i;
            a[j3 + 1] = wk3r * x0i - wk3i * x0r;
            x0r = y1r + y3i;
            x0i = y1i - y3r;
            a[j3 + 2] = wd3r * x0r + wd3i * x0i;
            a[j3 + 3] = wd3r * x0i - wd3i * x0r;
            j0 = m - j;
            j1 = j0 + m;
            j2 = j1 + m;
            j3 = j2 + m;
            x0r = a[j0] + a[j2];
            x0i = a[j0 + 1] + a[j2 + 1];
            x1r = a[j0] - a[j2];
            x1i = a[j0 + 1] - a[j2 + 1];
            y0r = a[j0 - 2] + a[j2 - 2];
            y0i = a[j0 - 1] + a[j2 - 1];
            y1r = a[j0 - 2] - a[j2 - 2];
            y1i = a[j0 - 1] - a[j2 - 1];
            x2r = a[j1] + a[j3];
            x2i = a[j1 + 1] + a[j3 + 1];
            x3r = a[j1] - a[j3];
            x3i = a[j1 + 1] - a[j3 + 1];
            y2r = a[j1 - 2] + a[j3 - 2];
            y2i = a[j1 - 1] + a[j3 - 1];
            y3r = a[j1 - 2] - a[j3 - 2];
            y3i = a[j1 - 1] - a[j3 - 1];
            a[j0] = x0r + x2r;
            a[j0 + 1] = x0i + x2i;
            a[j0 - 2] = y0r + y2r;
            a[j0 - 1] = y0i + y2i;
            a[j1] = x0r - x2r;
            a[j1 + 1] = x0i - x2i;
            a[j1 - 2] = y0r - y2r;
            a[j1 - 1] = y0i - y2i;
            x0r = x1r - x3i;
            x0i = x1i + x3r;
            a[j2] = wk1i * x0r - wk1r * x0i;
            a[j2 + 1] = wk1i * x0i + wk1r * x0r;
            x0r = y1r - y3i;
            x0i = y1i + y3r;
            a[j2 - 2] = wd1i * x0r - wd1r * x0i;
            a[j2 - 1] = wd1i * x0i + wd1r * x0r;
            x0r = x1r + x3i;
            x0i = x1i - x3r;
            a[j3] = wk3i * x0r + wk3r * x0i;
            a[j3 + 1] = wk3i * x0i - wk3r * x0r;
            x0r = y1r + y3i;
            x0i = y1i - y3r;
            a[j3 - 2] = wd3i * x0r + wd3r * x0i;
            a[j3 - 1] = wd3i * x0i - wd3r * x0r;
        }
        wk1r = csc1 * (wd1r + wn4r);
        wk1i = csc1 * (wd1i + wn4r);
        wk3r = csc3 * (wd3r - wn4r);
        wk3i = csc3 * (wd3i - wn4r);
        j0 = mh;
        j1 = j0 + m;
        j2 = j1 + m;
        j3 = j2 + m;
        x0r = a[j0 - 2] + a[j2 - 2];
        x0i = a[j0 - 1] + a[j2 - 1];
        x1r = a[j0 - 2] - a[j2 - 2];
        x1i = a[j0 - 1] - a[j2 - 1];
        x2r = a[j1 - 2] + a[j3 - 2];
        x2i = a[j1 - 1] + a[j3 - 1];
        x3r = a[j1 - 2] - a[j3 - 2];
        x3i = a[j1 - 1] - a[j3 - 1];
        a[j0 - 2] = x0r + x2r;
        a[j0 - 1] = x0i + x2i;
        a[j1 - 2] = x0r - x2r;
        a[j1 - 1] = x0i - x2i;
        x0r = x1r - x3i;
        x0i = x1i + x3r;
        a[j2 - 2] = wk1r * x0r - wk1i * x0i;
        a[j2 - 1] = wk1r * x0i + wk1i * x0r;
        x0r = x1r + x3i;
        x0i = x1i - x3r;
        a[j3 - 2] = wk3r * x0r + wk3i * x0i;
        a[j3 - 1] = wk3r * x0i - wk3i * x0r;
        x0r = a[j0] + a[j2];
        x0i = a[j0 + 1] + a[j2 + 1];
        x1r = a[j0] - a[j2];
        x1i = a[j0 + 1] - a[j2 + 1];
        x2r = a[j1] + a[j3];
        x2i = a[j1 + 1] + a[j3 + 1];
        x3r = a[j1] - a[j3];
        x3i = a[j1 + 1] - a[j3 + 1];
        a[j0] = x0r + x2r;
        a[j0 + 1] = x0i + x2i;
        a[j1] = x0r - x2r;
        a[j1 + 1] = x0i - x2i;
        x0r = x1r - x3i;
        x0i = x1i + x3r;
        a[j2] = wn4r * (x0r - x0i);
        a[j2 + 1] = wn4r * (x0i + x0r);
        x0r = x1r + x3i;
        x0i = x1i - x3r;
        a[j3] = -wn4r * (x0r + x0i);
        a[j3 + 1] = -wn4r * (x0i - x0r);
        x0r = a[j0 + 2] + a[j2 + 2];
        x0i = a[j0 + 3] + a[j2 + 3];
        x1r = a[j0 + 2] - a[j2 + 2];
        x1i = a[j0 + 3] - a[j2 + 3];
        x2r = a[j1 + 2] + a[j3 + 2];
        x2i = a[j1 + 3] + a[j3 + 3];
        x3r = a[j1 + 2] - a[j3 + 2];
        x3i = a[j1 + 3] - a[j3 + 3];
        a[j0 + 2] = x0r + x2r;
        a[j0 + 3] = x0i + x2i;
        a[j1 + 2] = x0r - x2r;
        a[j1 + 3] = x0i - x2i;
        x0r = x1r - x3i;
        x0i = x1i + x3r;
        a[j2 + 2] = wk1i * x0r - wk1r * x0i;
        a[j2 + 3] = wk1i * x0i + wk1r * x0r;
        x0r = x1r + x3i;
        x0i = x1i - x3r;
        a[j3 + 2] = wk3i * x0r + wk3r * x0i;
        a[j3 + 3] = wk3i * x0i - wk3r * x0r;
    }

    static void cftb1st(int n, double* a, double* w)
    {
        int j, j0, j1, j2, j3, k, m, mh;
        double wn4r, csc1, csc3, wk1r, wk1i, wk3r, wk3i,
            wd1r, wd1i, wd3r, wd3i;
        double x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i,
            y0r, y0i, y1r, y1i, y2r, y2i, y3r, y3i;

        mh = n >> 3;
        m = 2 * mh;
        j1 = m;
        j2 = j1 + m;
        j3 = j2 + m;
        x0r = a[0] + a[j2];
        x0i = -a[1] - a[j2 + 1];
        x1r = a[0] - a[j2];
        x1i = -a[1] + a[j2 + 1];
        x2r = a[j1] + a[j3];
        x2i = a[j1 + 1] + a[j3 + 1];
        x3r = a[j1] - a[j3];
        x3i = a[j1 + 1] - a[j3 + 1];
        a[0] = x0r + x2r;
        a[1] = x0i - x2i;
        a[j1] = x0r - x2r;
        a[j1 + 1] = x0i + x2i;
        a[j2] = x1r + x3i;
        a[j2 + 1] = x1i + x3r;
        a[j3] = x1r - x3i;
        a[j3 + 1] = x1i - x3r;
        wn4r = w[1];
        csc1 = w[2];
        csc3 = w[3];
        wd1r = 1;
        wd1i = 0;
        wd3r = 1;
        wd3i = 0;
        k = 0;
        for (j = 2; j < mh - 2; j += 4) {
            k += 4;
            wk1r = csc1 * (wd1r + w[k]);
            wk1i = csc1 * (wd1i + w[k + 1]);
            wk3r = csc3 * (wd3r + w[k + 2]);
            wk3i = csc3 * (wd3i + w[k + 3]);
            wd1r = w[k];
            wd1i = w[k + 1];
            wd3r = w[k + 2];
            wd3i = w[k + 3];
            j1 = j + m;
            j2 = j1 + m;
            j3 = j2 + m;
            x0r = a[j] + a[j2];
            x0i = -a[j + 1] - a[j2 + 1];
            x1r = a[j] - a[j2];
            x1i = -a[j + 1] + a[j2 + 1];
            y0r = a[j + 2] + a[j2 + 2];
            y0i = -a[j + 3] - a[j2 + 3];
            y1r = a[j + 2] - a[j2 + 2];
            y1i = -a[j + 3] + a[j2 + 3];
            x2r = a[j1] + a[j3];
            x2i = a[j1 + 1] + a[j3 + 1];
            x3r = a[j1] - a[j3];
            x3i = a[j1 + 1] - a[j3 + 1];
            y2r = a[j1 + 2] + a[j3 + 2];
            y2i = a[j1 + 3] + a[j3 + 3];
            y3r = a[j1 + 2] - a[j3 + 2];
            y3i = a[j1 + 3] - a[j3 + 3];
            a[j] = x0r + x2r;
            a[j + 1] = x0i - x2i;
            a[j + 2] = y0r + y2r;
            a[j + 3] = y0i - y2i;
            a[j1] = x0r - x2r;
            a[j1 + 1] = x0i + x2i;
            a[j1 + 2] = y0r - y2r;
            a[j1 + 3] = y0i + y2i;
            x0r = x1r + x3i;
            x0i = x1i + x3r;
            a[j2] = wk1r * x0r - wk1i * x0i;
            a[j2 + 1] = wk1r * x0i + wk1i * x0r;
            x0r = y1r + y3i;
            x0i = y1i + y3r;
            a[j2 + 2] = wd1r * x0r - wd1i * x0i;
            a[j2 + 3] = wd1r * x0i + wd1i * x0r;
            x0r = x1r - x3i;
            x0i = x1i - x3r;
            a[j3] = wk3r * x0r + wk3i * x0i;
            a[j3 + 1] = wk3r * x0i - wk3i * x0r;
            x0r = y1r - y3i;
            x0i = y1i - y3r;
            a[j3 + 2] = wd3r * x0r + wd3i * x0i;
            a[j3 + 3] = wd3r * x0i - wd3i * x0r;
            j0 = m - j;
            j1 = j0 + m;
            j2 = j1 + m;
            j3 = j2 + m;
            x0r = a[j0] + a[j2];
            x0i = -a[j0 + 1] - a[j2 + 1];
            x1r = a[j0] - a[j2];
            x1i = -a[j0 + 1] + a[j2 + 1];
            y0r = a[j0 - 2] + a[j2 - 2];
            y0i = -a[j0 - 1] - a[j2 - 1];
            y1r = a[j0 - 2] - a[j2 - 2];
            y1i = -a[j0 - 1] + a[j2 - 1];
            x2r = a[j1] + a[j3];
            x2i = a[j1 + 1] + a[j3 + 1];
            x3r = a[j1] - a[j3];
            x3i = a[j1 + 1] - a[j3 + 1];
            y2r = a[j1 - 2] + a[j3 - 2];
            y2i = a[j1 - 1] + a[j3 - 1];
            y3r = a[j1 - 2] - a[j3 - 2];
            y3i = a[j1 - 1] - a[j3 - 1];
            a[j0] = x0r + x2r;
            a[j0 + 1] = x0i - x2i;
            a[j0 - 2] = y0r + y2r;
            a[j0 - 1] = y0i - y2i;
            a[j1] = x0r - x2r;
            a[j1 + 1] = x0i + x2i;
            a[j1 - 2] = y0r - y2r;
            a[j1 - 1] = y0i + y2i;
            x0r = x1r + x3i;
            x0i = x1i + x3r;
            a[j2] = wk1i * x0r - wk1r * x0i;
            a[j2 + 1] = wk1i * x0i + wk1r * x0r;
            x0r = y1r + y3i;
            x0i = y1i + y3r;
            a[j2 - 2] = wd1i * x0r - wd1r * x0i;
            a[j2 - 1] = wd1i * x0i + wd1r * x0r;
            x0r = x1r - x3i;
            x0i = x1i - x3r;
            a[j3] = wk3i * x0r + wk3r * x0i;
            a[j3 + 1] = wk3i * x0i - wk3r * x0r;
            x0r = y1r - y3i;
            x0i = y1i - y3r;
            a[j3 - 2] = wd3i * x0r + wd3r * x0i;
            a[j3 - 1] = wd3i * x0i - wd3r * x0r;
        }
        wk1r = csc1 * (wd1r + wn4r);
        wk1i = csc1 * (wd1i + wn4r);
        wk3r = csc3 * (wd3r - wn4r);
        wk3i = csc3 * (wd3i - wn4r);
        j0 = mh;
        j1 = j0 + m;
        j2 = j1 + m;
        j3 = j2 + m;
        x0r = a[j0 - 2] + a[j2 - 2];
        x0i = -a[j0 - 1] - a[j2 - 1];
        x1r = a[j0 - 2] - a[j2 - 2];
        x1i = -a[j0 - 1] + a[j2 - 1];
        x2r = a[j1 - 2] + a[j3 - 2];
        x2i = a[j1 - 1] + a[j3 - 1];
        x3r = a[j1 - 2] - a[j3 - 2];
        x3i = a[j1 - 1] - a[j3 - 1];
        a[j0 - 2] = x0r + x2r;
        a[j0 - 1] = x0i - x2i;
        a[j1 - 2] = x0r - x2r;
        a[j1 - 1] = x0i + x2i;
        x0r = x1r + x3i;
        x0i = x1i + x3r;
        a[j2 - 2] = wk1r * x0r - wk1i * x0i;
        a[j2 - 1] = wk1r * x0i + wk1i * x0r;
        x0r = x1r - x3i;
        x0i = x1i - x3r;
        a[j3 - 2] = wk3r * x0r + wk3i * x0i;
        a[j3 - 1] = wk3r * x0i - wk3i * x0r;
        x0r = a[j0] + a[j2];
        x0i = -a[j0 + 1] - a[j2 + 1];
        x1r = a[j0] - a[j2];
        x1i = -a[j0 + 1] + a[j2 + 1];
        x2r = a[j1] + a[j3];
        x2i = a[j1 + 1] + a[j3 + 1];
        x3r = a[j1] - a[j3];
        x3i = a[j1 + 1] - a[j3 + 1];
        a[j0] = x0r + x2r;
        a[j0 + 1] = x0i - x2i;
        a[j1] = x0r - x2r;
        a[j1 + 1] = x0i + x2i;
        x0r = x1r + x3i;
        x0i = x1i + x3r;
        a[j2] = wn4r * (x0r - x0i);
        a[j2 + 1] = wn4r * (x0i + x0r);
        x0r = x1r - x3i;
        x0i = x1i - x3r;
        a[j3] = -wn4r * (x0r + x0i);
        a[j3 + 1] = -wn4r * (x0i - x0r);
        x0r = a[j0 + 2] + a[j2 + 2];
        x0i = -a[j0 + 3] - a[j2 + 3];
        x1r = a[j0 + 2] - a[j2 + 2];
        x1i = -a[j0 + 3] + a[j2 + 3];
        x2r = a[j1 + 2] + a[j3 + 2];
        x2i = a[j1 + 3] + a[j3 + 3];
        x3r = a[j1 + 2] - a[j3 + 2];
        x3i = a[j1 + 3] - a[j3 + 3];
        a[j0 + 2] = x0r + x2r;
        a[j0 + 3] = x0i - x2i;
        a[j1 + 2] = x0r - x2r;
        a[j1 + 3] = x0i + x2i;
        x0r = x1r + x3i;
        x0i = x1i + x3r;
        a[j2 + 2] = wk1i * x0r - wk1r * x0i;
        a[j2 + 3] = wk1i * x0i + wk1r * x0r;
        x0r = x1r - x3i;
        x0i = x1i - x3r;
        a[j3 + 2] = wk3i * x0r + wk3r * x0i;
        a[j3 + 3] = wk3i * x0i - wk3r * x0r;
    }

    static void cftmdl1(int n, double* a, double* w)
    {
        int j, j0, j1, j2, j3, k, m, mh;
        double wn4r, wk1r, wk1i, wk3r, wk3i;
        double x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;

        mh = n >> 3;
        m = 2 * mh;
        j1 = m;
        j2 = j1 + m;
        j3 = j2 + m;
        x0r = a[0] + a[j2];
        x0i = a[1] + a[j2 + 1];
        x1r = a[0] - a[j2];
        x1i = a[1] - a[j2 + 1];
        x2r = a[j1] + a[j3];
        x2i = a[j1 + 1] + a[j3 + 1];
        x3r = a[j1] - a[j3];
        x3i = a[j1 + 1] - a[j3 + 1];
        a[0] = x0r + x2r;
        a[1] = x0i + x2i;
        a[j1] = x0r - x2r;
        a[j1 + 1] = x0i - x2i;
        a[j2] = x1r - x3i;
        a[j2 + 1] = x1i + x3r;
        a[j3] = x1r + x3i;
        a[j3 + 1] = x1i - x3r;
        wn4r = w[1];
        k = 0;
        for (j = 2; j < mh; j += 2) {
            k += 4;
            wk1r = w[k];
            wk1i = w[k + 1];
            wk3r = w[k + 2];
            wk3i = w[k + 3];
            j1 = j + m;
            j2 = j1 + m;
            j3 = j2 + m;
            x0r = a[j] + a[j2];
            x0i = a[j + 1] + a[j2 + 1];
            x1r = a[j] - a[j2];
            x1i = a[j + 1] - a[j2 + 1];
            x2r = a[j1] + a[j3];
            x2i = a[j1 + 1] + a[j3 + 1];
            x3r = a[j1] - a[j3];
            x3i = a[j1 + 1] - a[j3 + 1];
            a[j] = x0r + x2r;
            a[j + 1] = x0i + x2i;
            a[j1] = x0r - x2r;
            a[j1 + 1] = x0i - x2i;
            x0r = x1r - x3i;
            x0i = x1i + x3r;
            a[j2] = wk1r * x0r - wk1i * x0i;
            a[j2 + 1] = wk1r * x0i + wk1i * x0r;
            x0r = x1r + x3i;
            x0i = x1i - x3r;
            a[j3] = wk3r * x0r + wk3i * x0i;
            a[j3 + 1] = wk3r * x0i - wk3i * x0r;
            j0 = m - j;
            j1 = j0 + m;
            j2 = j1 + m;
            j3 = j2 + m;
            x0r = a[j0] + a[j2];
            x0i = a[j0 + 1] + a[j2 + 1];
            x1r = a[j0] - a[j2];
            x1i = a[j0 + 1] - a[j2 + 1];
            x2r = a[j1] + a[j3];
            x2i = a[j1 + 1] + a[j3 + 1];
            x3r = a[j1] - a[j3];
            x3i = a[j1 + 1] - a[j3 + 1];
            a[j0] = x0r + x2r;
            a[j0 + 1] = x0i + x2i;
            a[j1] = x0r - x2r;
            a[j1 + 1] = x0i - x2i;
            x0r = x1r - x3i;
            x0i = x1i + x3r;
            a[j2] = wk1i * x0r - wk1r * x0i;
            a[j2 + 1] = wk1i * x0i + wk1r * x0r;
            x0r = x1r + x3i;
            x0i = x1i - x3r;
            a[j3] = wk3i * x0r + wk3r * x0i;
            a[j3 + 1] = wk3i * x0i - wk3r * x0r;
        }
        j0 = mh;
        j1 = j0 + m;
        j2 = j1 + m;
        j3 = j2 + m;
        x0r = a[j0] + a[j2];
        x0i = a[j0 + 1] + a[j2 + 1];
        x1r = a[j0] - a[j2];
        x1i = a[j0 + 1] - a[j2 + 1];
        x2r = a[j1] + a[j3];
        x2i = a[j1 + 1] + a[j3 + 1];
        x3r = a[j1] - a[j3];
        x3i = a[j1 + 1] - a[j3 + 1];
        a[j0] = x0r + x2r;
        a[j0 + 1] = x0i + x2i;
        a[j1] = x0r - x2r;
        a[j1 + 1] = x0i - x2i;
        x0r = x1r - x3i;
        x0i = x1i + x3r;
        a[j2] = wn4r * (x0r - x0i);
        a[j2 + 1] = wn4r * (x0i + x0r);
        x0r = x1r + x3i;
        x0i = x1i - x3r;
        a[j3] = -wn4r * (x0r + x0i);
        a[j3 + 1] = -wn4r * (x0i - x0r);
    }

    static void cftmdl2(int n, double* a, double* w)
    {
        int j, j0, j1, j2, j3, k, kr, m, mh;
        double wn4r, wk1r, wk1i, wk3r, wk3i, wd1r, wd1i, wd3r, wd3i;
        double x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i, y0r, y0i, y2r, y2i;

        mh = n >> 3;
        m = 2 * mh;
        wn4r = w[1];
        j1 = m;
        j2 = j1 + m;
        j3 = j2 + m;
        x0r = a[0] - a[j2 + 1];
        x0i = a[1] + a[j2];
        x1r = a[0] + a[j2 + 1];
        x1i = a[1] - a[j2];
        x2r = a[j1] - a[j3 + 1];
        x2i = a[j1 + 1] + a[j3];
        x3r = a[j1] + a[j3 + 1];
        x3i = a[j1 + 1] - a[j3];
        y0r = wn4r * (x2r - x2i);
        y0i = wn4r * (x2i + x2r);
        a[0] = x0r + y0r;
        a[1] = x0i + y0i;
        a[j1] = x0r - y0r;
        a[j1 + 1] = x0i - y0i;
        y0r = wn4r * (x3r - x3i);
        y0i = wn4r * (x3i + x3r);
        a[j2] = x1r - y0i;
        a[j2 + 1] = x1i + y0r;
        a[j3] = x1r + y0i;
        a[j3 + 1] = x1i - y0r;
        k = 0;
        kr = 2 * m;
        for (j = 2; j < mh; j += 2) {
            k += 4;
            wk1r = w[k];
            wk1i = w[k + 1];
            wk3r = w[k + 2];
            wk3i = w[k + 3];
            kr -= 4;
            wd1i = w[kr];
            wd1r = w[kr + 1];
            wd3i = w[kr + 2];
            wd3r = w[kr + 3];
            j1 = j + m;
            j2 = j1 + m;
            j3 = j2 + m;
            x0r = a[j] - a[j2 + 1];
            x0i = a[j + 1] + a[j2];
            x1r = a[j] + a[j2 + 1];
            x1i = a[j + 1] - a[j2];
            x2r = a[j1] - a[j3 + 1];
            x2i = a[j1 + 1] + a[j3];
            x3r = a[j1] + a[j3 + 1];
            x3i = a[j1 + 1] - a[j3];
            y0r = wk1r * x0r - wk1i * x0i;
            y0i = wk1r * x0i + wk1i * x0r;
            y2r = wd1r * x2r - wd1i * x2i;
            y2i = wd1r * x2i + wd1i * x2r;
            a[j] = y0r + y2r;
            a[j + 1] = y0i + y2i;
            a[j1] = y0r - y2r;
            a[j1 + 1] = y0i - y2i;
            y0r = wk3r * x1r + wk3i * x1i;
            y0i = wk3r * x1i - wk3i * x1r;
            y2r = wd3r * x3r + wd3i * x3i;
            y2i = wd3r * x3i - wd3i * x3r;
            a[j2] = y0r + y2r;
            a[j2 + 1] = y0i + y2i;
            a[j3] = y0r - y2r;
            a[j3 + 1] = y0i - y2i;
            j0 = m - j;
            j1 = j0 + m;
            j2 = j1 + m;
            j3 = j2 + m;
            x0r = a[j0] - a[j2 + 1];
            x0i = a[j0 + 1] + a[j2];
            x1r = a[j0] + a[j2 + 1];
            x1i = a[j0 + 1] - a[j2];
            x2r = a[j1] - a[j3 + 1];
            x2i = a[j1 + 1] + a[j3];
            x3r = a[j1] + a[j3 + 1];
            x3i = a[j1 + 1] - a[j3];
            y0r = wd1i * x0r - wd1r * x0i;
            y0i = wd1i * x0i + wd1r * x0r;
            y2r = wk1i * x2r - wk1r * x2i;
            y2i = wk1i * x2i + wk1r * x2r;
            a[j0] = y0r + y2r;
            a[j0 + 1] = y0i + y2i;
            a[j1] = y0r - y2r;
            a[j1 + 1] = y0i - y2i;
            y0r = wd3i * x1r + wd3r * x1i;
            y0i = wd3i * x1i - wd3r * x1r;
            y2r = wk3i * x3r + wk3r * x3i;
            y2i = wk3i * x3i - wk3r * x3r;
            a[j2] = y0r + y2r;
            a[j2 + 1] = y0i + y2i;
            a[j3] = y0r - y2r;
            a[j3 + 1] = y0i - y2i;
        }
        wk1r = w[m];
        wk1i = w[m + 1];
        j0 = mh;
        j1 = j0 + m;
        j2 = j1 + m;
        j3 = j2 + m;
        x0r = a[j0] - a[j2 + 1];
        x0i = a[j0 + 1] + a[j2];
        x1r = a[j0] + a[j2 + 1];
        x1i = a[j0 + 1] - a[j2];
        x2r = a[j1] - a[j3 + 1];
        x2i = a[j1 + 1] + a[j3];
        x3r = a[j1] + a[j3 + 1];
        x3i = a[j1 + 1] - a[j3];
        y0r = wk1r * x0r - wk1i * x0i;
        y0i = wk1r * x0i + wk1i * x0r;
        y2r = wk1i * x2r - wk1r * x2i;
        y2i = wk1i * x2i + wk1r * x2r;
        a[j0] = y0r + y2r;
        a[j0 + 1] = y0i + y2i;
        a[j1] = y0r - y2r;
        a[j1 + 1] = y0i - y2i;
        y0r = wk1i * x1r - wk1r * x1i;
        y0i = wk1i * x1i + wk1r * x1r;
        y2r = wk1r * x3r - wk1i * x3i;
        y2i = wk1r * x3i + wk1i * x3r;
        a[j2] = y0r - y2r;
        a[j2 + 1] = y0i - y2i;
        a[j3] = y0r + y2r;
        a[j3 + 1] = y0i + y2i;
    }

    static int cfttree(int n, int j, int k, double* a, int nw, double* w)
    {
        int i, isplt, m;

        if ((k & 3) != 0) {
            isplt = k & 1;
            if (isplt != 0) {
                cftmdl1(n, &a[j - n], &w[nw - (n >> 1)]);
            }
            else {
                cftmdl2(n, &a[j - n], &w[nw - n]);
            }
        }
        else {
            m = n;
            for (i = k; (i & 3) == 0; i >>= 2) {
                m <<= 2;
            }
            isplt = i & 1;
            if (isplt != 0) {
                while (m > 128) {
                    cftmdl1(m, &a[j - m], &w[nw - (m >> 1)]);
                    m >>= 2;
                }
            }
            else {
                while (m > 128) {
                    cftmdl2(m, &a[j - m], &w[nw - m]);
                    m >>= 2;
                }
            }
        }
        return isplt;
    }

    static void cftf161(double* a, double* w)
    {
        double wn4r, wk1r, wk1i,
            x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i,
            y0r, y0i, y1r, y1i, y2r, y2i, y3r, y3i,
            y4r, y4i, y5r, y5i, y6r, y6i, y7r, y7i,
            y8r, y8i, y9r, y9i, y10r, y10i, y11r, y11i,
            y12r, y12i, y13r, y13i, y14r, y14i, y15r, y15i;

        wn4r = w[1];
        wk1r = w[2];
        wk1i = w[3];
        x0r = a[0] + a[16];
        x0i = a[1] + a[17];
        x1r = a[0] - a[16];
        x1i = a[1] - a[17];
        x2r = a[8] + a[24];
        x2i = a[9] + a[25];
        x3r = a[8] - a[24];
        x3i = a[9] - a[25];
        y0r = x0r + x2r;
        y0i = x0i + x2i;
        y4r = x0r - x2r;
        y4i = x0i - x2i;
        y8r = x1r - x3i;
        y8i = x1i + x3r;
        y12r = x1r + x3i;
        y12i = x1i - x3r;
        x0r = a[2] + a[18];
        x0i = a[3] + a[19];
        x1r = a[2] - a[18];
        x1i = a[3] - a[19];
        x2r = a[10] + a[26];
        x2i = a[11] + a[27];
        x3r = a[10] - a[26];
        x3i = a[11] - a[27];
        y1r = x0r + x2r;
        y1i = x0i + x2i;
        y5r = x0r - x2r;
        y5i = x0i - x2i;
        x0r = x1r - x3i;
        x0i = x1i + x3r;
        y9r = wk1r * x0r - wk1i * x0i;
        y9i = wk1r * x0i + wk1i * x0r;
        x0r = x1r + x3i;
        x0i = x1i - x3r;
        y13r = wk1i * x0r - wk1r * x0i;
        y13i = wk1i * x0i + wk1r * x0r;
        x0r = a[4] + a[20];
        x0i = a[5] + a[21];
        x1r = a[4] - a[20];
        x1i = a[5] - a[21];
        x2r = a[12] + a[28];
        x2i = a[13] + a[29];
        x3r = a[12] - a[28];
        x3i = a[13] - a[29];
        y2r = x0r + x2r;
        y2i = x0i + x2i;
        y6r = x0r - x2r;
        y6i = x0i - x2i;
        x0r = x1r - x3i;
        x0i = x1i + x3r;
        y10r = wn4r * (x0r - x0i);
        y10i = wn4r * (x0i + x0r);
        x0r = x1r + x3i;
        x0i = x1i - x3r;
        y14r = wn4r * (x0r + x0i);
        y14i = wn4r * (x0i - x0r);
        x0r = a[6] + a[22];
        x0i = a[7] + a[23];
        x1r = a[6] - a[22];
        x1i = a[7] - a[23];
        x2r = a[14] + a[30];
        x2i = a[15] + a[31];
        x3r = a[14] - a[30];
        x3i = a[15] - a[31];
        y3r = x0r + x2r;
        y3i = x0i + x2i;
        y7r = x0r - x2r;
        y7i = x0i - x2i;
        x0r = x1r - x3i;
        x0i = x1i + x3r;
        y11r = wk1i * x0r - wk1r * x0i;
        y11i = wk1i * x0i + wk1r * x0r;
        x0r = x1r + x3i;
        x0i = x1i - x3r;
        y15r = wk1r * x0r - wk1i * x0i;
        y15i = wk1r * x0i + wk1i * x0r;
        x0r = y12r - y14r;
        x0i = y12i - y14i;
        x1r = y12r + y14r;
        x1i = y12i + y14i;
        x2r = y13r - y15r;
        x2i = y13i - y15i;
        x3r = y13r + y15r;
        x3i = y13i + y15i;
        a[24] = x0r + x2r;
        a[25] = x0i + x2i;
        a[26] = x0r - x2r;
        a[27] = x0i - x2i;
        a[28] = x1r - x3i;
        a[29] = x1i + x3r;
        a[30] = x1r + x3i;
        a[31] = x1i - x3r;
        x0r = y8r + y10r;
        x0i = y8i + y10i;
        x1r = y8r - y10r;
        x1i = y8i - y10i;
        x2r = y9r + y11r;
        x2i = y9i + y11i;
        x3r = y9r - y11r;
        x3i = y9i - y11i;
        a[16] = x0r + x2r;
        a[17] = x0i + x2i;
        a[18] = x0r - x2r;
        a[19] = x0i - x2i;
        a[20] = x1r - x3i;
        a[21] = x1i + x3r;
        a[22] = x1r + x3i;
        a[23] = x1i - x3r;
        x0r = y5r - y7i;
        x0i = y5i + y7r;
        x2r = wn4r * (x0r - x0i);
        x2i = wn4r * (x0i + x0r);
        x0r = y5r + y7i;
        x0i = y5i - y7r;
        x3r = wn4r * (x0r - x0i);
        x3i = wn4r * (x0i + x0r);
        x0r = y4r - y6i;
        x0i = y4i + y6r;
        x1r = y4r + y6i;
        x1i = y4i - y6r;
        a[8] = x0r + x2r;
        a[9] = x0i + x2i;
        a[10] = x0r - x2r;
        a[11] = x0i - x2i;
        a[12] = x1r - x3i;
        a[13] = x1i + x3r;
        a[14] = x1r + x3i;
        a[15] = x1i - x3r;
        x0r = y0r + y2r;
        x0i = y0i + y2i;
        x1r = y0r - y2r;
        x1i = y0i - y2i;
        x2r = y1r + y3r;
        x2i = y1i + y3i;
        x3r = y1r - y3r;
        x3i = y1i - y3i;
        a[0] = x0r + x2r;
        a[1] = x0i + x2i;
        a[2] = x0r - x2r;
        a[3] = x0i - x2i;
        a[4] = x1r - x3i;
        a[5] = x1i + x3r;
        a[6] = x1r + x3i;
        a[7] = x1i - x3r;
    }

    static void cftf162(double* a, double* w)
    {
        double wn4r, wk1r, wk1i, wk2r, wk2i, wk3r, wk3i,
            x0r, x0i, x1r, x1i, x2r, x2i,
            y0r, y0i, y1r, y1i, y2r, y2i, y3r, y3i,
            y4r, y4i, y5r, y5i, y6r, y6i, y7r, y7i,
            y8r, y8i, y9r, y9i, y10r, y10i, y11r, y11i,
            y12r, y12i, y13r, y13i, y14r, y14i, y15r, y15i;

        wn4r = w[1];
        wk1r = w[4];
        wk1i = w[5];
        wk3r = w[6];
        wk3i = -w[7];
        wk2r = w[8];
        wk2i = w[9];
        x1r = a[0] - a[17];
        x1i = a[1] + a[16];
        x0r = a[8] - a[25];
        x0i = a[9] + a[24];
        x2r = wn4r * (x0r - x0i);
        x2i = wn4r * (x0i + x0r);
        y0r = x1r + x2r;
        y0i = x1i + x2i;
        y4r = x1r - x2r;
        y4i = x1i - x2i;
        x1r = a[0] + a[17];
        x1i = a[1] - a[16];
        x0r = a[8] + a[25];
        x0i = a[9] - a[24];
        x2r = wn4r * (x0r - x0i);
        x2i = wn4r * (x0i + x0r);
        y8r = x1r - x2i;
        y8i = x1i + x2r;
        y12r = x1r + x2i;
        y12i = x1i - x2r;
        x0r = a[2] - a[19];
        x0i = a[3] + a[18];
        x1r = wk1r * x0r - wk1i * x0i;
        x1i = wk1r * x0i + wk1i * x0r;
        x0r = a[10] - a[27];
        x0i = a[11] + a[26];
        x2r = wk3i * x0r - wk3r * x0i;
        x2i = wk3i * x0i + wk3r * x0r;
        y1r = x1r + x2r;
        y1i = x1i + x2i;
        y5r = x1r - x2r;
        y5i = x1i - x2i;
        x0r = a[2] + a[19];
        x0i = a[3] - a[18];
        x1r = wk3r * x0r - wk3i * x0i;
        x1i = wk3r * x0i + wk3i * x0r;
        x0r = a[10] + a[27];
        x0i = a[11] - a[26];
        x2r = wk1r * x0r + wk1i * x0i;
        x2i = wk1r * x0i - wk1i * x0r;
        y9r = x1r - x2r;
        y9i = x1i - x2i;
        y13r = x1r + x2r;
        y13i = x1i + x2i;
        x0r = a[4] - a[21];
        x0i = a[5] + a[20];
        x1r = wk2r * x0r - wk2i * x0i;
        x1i = wk2r * x0i + wk2i * x0r;
        x0r = a[12] - a[29];
        x0i = a[13] + a[28];
        x2r = wk2i * x0r - wk2r * x0i;
        x2i = wk2i * x0i + wk2r * x0r;
        y2r = x1r + x2r;
        y2i = x1i + x2i;
        y6r = x1r - x2r;
        y6i = x1i - x2i;
        x0r = a[4] + a[21];
        x0i = a[5] - a[20];
        x1r = wk2i * x0r - wk2r * x0i;
        x1i = wk2i * x0i + wk2r * x0r;
        x0r = a[12] + a[29];
        x0i = a[13] - a[28];
        x2r = wk2r * x0r - wk2i * x0i;
        x2i = wk2r * x0i + wk2i * x0r;
        y10r = x1r - x2r;
        y10i = x1i - x2i;
        y14r = x1r + x2r;
        y14i = x1i + x2i;
        x0r = a[6] - a[23];
        x0i = a[7] + a[22];
        x1r = wk3r * x0r - wk3i * x0i;
        x1i = wk3r * x0i + wk3i * x0r;
        x0r = a[14] - a[31];
        x0i = a[15] + a[30];
        x2r = wk1i * x0r - wk1r * x0i;
        x2i = wk1i * x0i + wk1r * x0r;
        y3r = x1r + x2r;
        y3i = x1i + x2i;
        y7r = x1r - x2r;
        y7i = x1i - x2i;
        x0r = a[6] + a[23];
        x0i = a[7] - a[22];
        x1r = wk1i * x0r + wk1r * x0i;
        x1i = wk1i * x0i - wk1r * x0r;
        x0r = a[14] + a[31];
        x0i = a[15] - a[30];
        x2r = wk3i * x0r - wk3r * x0i;
        x2i = wk3i * x0i + wk3r * x0r;
        y11r = x1r + x2r;
        y11i = x1i + x2i;
        y15r = x1r - x2r;
        y15i = x1i - x2i;
        x1r = y0r + y2r;
        x1i = y0i + y2i;
        x2r = y1r + y3r;
        x2i = y1i + y3i;
        a[0] = x1r + x2r;
        a[1] = x1i + x2i;
        a[2] = x1r - x2r;
        a[3] = x1i - x2i;
        x1r = y0r - y2r;
        x1i = y0i - y2i;
        x2r = y1r - y3r;
        x2i = y1i - y3i;
        a[4] = x1r - x2i;
        a[5] = x1i + x2r;
        a[6] = x1r + x2i;
        a[7] = x1i - x2r;
        x1r = y4r - y6i;
        x1i = y4i + y6r;
        x0r = y5r - y7i;
        x0i = y5i + y7r;
        x2r = wn4r * (x0r - x0i);
        x2i = wn4r * (x0i + x0r);
        a[8] = x1r + x2r;
        a[9] = x1i + x2i;
        a[10] = x1r - x2r;
        a[11] = x1i - x2i;
        x1r = y4r + y6i;
        x1i = y4i - y6r;
        x0r = y5r + y7i;
        x0i = y5i - y7r;
        x2r = wn4r * (x0r - x0i);
        x2i = wn4r * (x0i + x0r);
        a[12] = x1r - x2i;
        a[13] = x1i + x2r;
        a[14] = x1r + x2i;
        a[15] = x1i - x2r;
        x1r = y8r + y10r;
        x1i = y8i + y10i;
        x2r = y9r - y11r;
        x2i = y9i - y11i;
        a[16] = x1r + x2r;
        a[17] = x1i + x2i;
        a[18] = x1r - x2r;
        a[19] = x1i - x2i;
        x1r = y8r - y10r;
        x1i = y8i - y10i;
        x2r = y9r + y11r;
        x2i = y9i + y11i;
        a[20] = x1r - x2i;
        a[21] = x1i + x2r;
        a[22] = x1r + x2i;
        a[23] = x1i - x2r;
        x1r = y12r - y14i;
        x1i = y12i + y14r;
        x0r = y13r + y15i;
        x0i = y13i - y15r;
        x2r = wn4r * (x0r - x0i);
        x2i = wn4r * (x0i + x0r);
        a[24] = x1r + x2r;
        a[25] = x1i + x2i;
        a[26] = x1r - x2r;
        a[27] = x1i - x2i;
        x1r = y12r + y14i;
        x1i = y12i - y14r;
        x0r = y13r - y15i;
        x0i = y13i + y15r;
        x2r = wn4r * (x0r - x0i);
        x2i = wn4r * (x0i + x0r);
        a[28] = x1r - x2i;
        a[29] = x1i + x2r;
        a[30] = x1r + x2i;
        a[31] = x1i - x2r;
    }

    static void cftf081(double* a, double* w)
    {
        double wn4r, x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i,
            y0r, y0i, y1r, y1i, y2r, y2i, y3r, y3i,
            y4r, y4i, y5r, y5i, y6r, y6i, y7r, y7i;

        wn4r = w[1];
        x0r = a[0] + a[8];
        x0i = a[1] + a[9];
        x1r = a[0] - a[8];
        x1i = a[1] - a[9];
        x2r = a[4] + a[12];
        x2i = a[5] + a[13];
        x3r = a[4] - a[12];
        x3i = a[5] - a[13];
        y0r = x0r + x2r;
        y0i = x0i + x2i;
        y2r = x0r - x2r;
        y2i = x0i - x2i;
        y1r = x1r - x3i;
        y1i = x1i + x3r;
        y3r = x1r + x3i;
        y3i = x1i - x3r;
        x0r = a[2] + a[10];
        x0i = a[3] + a[11];
        x1r = a[2] - a[10];
        x1i = a[3] - a[11];
        x2r = a[6] + a[14];
        x2i = a[7] + a[15];
        x3r = a[6] - a[14];
        x3i = a[7] - a[15];
        y4r = x0r + x2r;
        y4i = x0i + x2i;
        y6r = x0r - x2r;
        y6i = x0i - x2i;
        x0r = x1r - x3i;
        x0i = x1i + x3r;
        x2r = x1r + x3i;
        x2i = x1i - x3r;
        y5r = wn4r * (x0r - x0i);
        y5i = wn4r * (x0r + x0i);
        y7r = wn4r * (x2r - x2i);
        y7i = wn4r * (x2r + x2i);
        a[8] = y1r + y5r;
        a[9] = y1i + y5i;
        a[10] = y1r - y5r;
        a[11] = y1i - y5i;
        a[12] = y3r - y7i;
        a[13] = y3i + y7r;
        a[14] = y3r + y7i;
        a[15] = y3i - y7r;
        a[0] = y0r + y4r;
        a[1] = y0i + y4i;
        a[2] = y0r - y4r;
        a[3] = y0i - y4i;
        a[4] = y2r - y6i;
        a[5] = y2i + y6r;
        a[6] = y2r + y6i;
        a[7] = y2i - y6r;
    }

    static void cftf082(double* a, double* w)
    {
        double wn4r, wk1r, wk1i, x0r, x0i, x1r, x1i,
            y0r, y0i, y1r, y1i, y2r, y2i, y3r, y3i,
            y4r, y4i, y5r, y5i, y6r, y6i, y7r, y7i;

        wn4r = w[1];
        wk1r = w[2];
        wk1i = w[3];
        y0r = a[0] - a[9];
        y0i = a[1] + a[8];
        y1r = a[0] + a[9];
        y1i = a[1] - a[8];
        x0r = a[4] - a[13];
        x0i = a[5] + a[12];
        y2r = wn4r * (x0r - x0i);
        y2i = wn4r * (x0i + x0r);
        x0r = a[4] + a[13];
        x0i = a[5] - a[12];
        y3r = wn4r * (x0r - x0i);
        y3i = wn4r * (x0i + x0r);
        x0r = a[2] - a[11];
        x0i = a[3] + a[10];
        y4r = wk1r * x0r - wk1i * x0i;
        y4i = wk1r * x0i + wk1i * x0r;
        x0r = a[2] + a[11];
        x0i = a[3] - a[10];
        y5r = wk1i * x0r - wk1r * x0i;
        y5i = wk1i * x0i + wk1r * x0r;
        x0r = a[6] - a[15];
        x0i = a[7] + a[14];
        y6r = wk1i * x0r - wk1r * x0i;
        y6i = wk1i * x0i + wk1r * x0r;
        x0r = a[6] + a[15];
        x0i = a[7] - a[14];
        y7r = wk1r * x0r - wk1i * x0i;
        y7i = wk1r * x0i + wk1i * x0r;
        x0r = y0r + y2r;
        x0i = y0i + y2i;
        x1r = y4r + y6r;
        x1i = y4i + y6i;
        a[0] = x0r + x1r;
        a[1] = x0i + x1i;
        a[2] = x0r - x1r;
        a[3] = x0i - x1i;
        x0r = y0r - y2r;
        x0i = y0i - y2i;
        x1r = y4r - y6r;
        x1i = y4i - y6i;
        a[4] = x0r - x1i;
        a[5] = x0i + x1r;
        a[6] = x0r + x1i;
        a[7] = x0i - x1r;
        x0r = y1r - y3i;
        x0i = y1i + y3r;
        x1r = y5r - y7r;
        x1i = y5i - y7i;
        a[8] = x0r + x1r;
        a[9] = x0i + x1i;
        a[10] = x0r - x1r;
        a[11] = x0i - x1i;
        x0r = y1r + y3i;
        x0i = y1i - y3r;
        x1r = y5r + y7r;
        x1i = y5i + y7i;
        a[12] = x0r - x1i;
        a[13] = x0i + x1r;
        a[14] = x0r + x1i;
        a[15] = x0i - x1r;
    }

    static void cftf040(double* a)
    {
        double x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;

        x0r = a[0] + a[4];
        x0i = a[1] + a[5];
        x1r = a[0] - a[4];
        x1i = a[1] - a[5];
        x2r = a[2] + a[6];
        x2i = a[3] + a[7];
        x3r = a[2] - a[6];
        x3i = a[3] - a[7];
        a[0] = x0r + x2r;
        a[1] = x0i + x2i;
        a[2] = x1r - x3i;
        a[3] = x1i + x3r;
        a[4] = x0r - x2r;
        a[5] = x0i - x2i;
        a[6] = x1r + x3i;
        a[7] = x1i - x3r;
    }

    static void cftb040(double* a)
    {
        double x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;

        x0r = a[0] + a[4];
        x0i = a[1] + a[5];
        x1r = a[0] - a[4];
        x1i = a[1] - a[5];
        x2r = a[2] + a[6];
        x2i = a[3] + a[7];
        x3r = a[2] - a[6];
        x3i = a[3] - a[7];
        a[0] = x0r + x2r;
        a[1] = x0i + x2i;
        a[2] = x1r + x3i;
        a[3] = x1i - x3r;
        a[4] = x0r - x2r;
        a[5] = x0i - x2i;
        a[6] = x1r - x3i;
        a[7] = x1i + x3r;
    }

    static void cftx020(double* a)
    {
        double x0r, x0i;

        x0r = a[0] - a[2];
        x0i = a[1] - a[3];
        a[0] += a[2];
        a[1] += a[3];
        a[2] = x0r;
        a[3] = x0i;
    }

    static void cftfx41(int n, double* a, int nw, double* w)
    {
        if (n == 128) {
            cftf161(a, &w[nw - 8]);
            cftf162(&a[32], &w[nw - 32]);
            cftf161(&a[64], &w[nw - 8]);
            cftf161(&a[96], &w[nw - 8]);
        }
        else {
            cftf081(a, &w[nw - 8]);
            cftf082(&a[16], &w[nw - 8]);
            cftf081(&a[32], &w[nw - 8]);
            cftf081(&a[48], &w[nw - 8]);
        }
    }

    static void cftleaf(int n, int isplt, double* a, int nw, double* w)
    {
        if (n == 512) {
            cftmdl1(128, a, &w[nw - 64]);
            cftf161(a, &w[nw - 8]);
            cftf162(&a[32], &w[nw - 32]);
            cftf161(&a[64], &w[nw - 8]);
            cftf161(&a[96], &w[nw - 8]);
            cftmdl2(128, &a[128], &w[nw - 128]);
            cftf161(&a[128], &w[nw - 8]);
            cftf162(&a[160], &w[nw - 32]);
            cftf161(&a[192], &w[nw - 8]);
            cftf162(&a[224], &w[nw - 32]);
            cftmdl1(128, &a[256], &w[nw - 64]);
            cftf161(&a[256], &w[nw - 8]);
            cftf162(&a[288], &w[nw - 32]);
            cftf161(&a[320], &w[nw - 8]);
            cftf161(&a[352], &w[nw - 8]);
            if (isplt != 0) {
                cftmdl1(128, &a[384], &w[nw - 64]);
                cftf161(&a[480], &w[nw - 8]);
            }
            else {
                cftmdl2(128, &a[384], &w[nw - 128]);
                cftf162(&a[480], &w[nw - 32]);
            }
            cftf161(&a[384], &w[nw - 8]);
            cftf162(&a[416], &w[nw - 32]);
            cftf161(&a[448], &w[nw - 8]);
        }
        else {
            cftmdl1(64, a, &w[nw - 32]);
            cftf081(a, &w[nw - 8]);
            cftf082(&a[16], &w[nw - 8]);
            cftf081(&a[32], &w[nw - 8]);
            cftf081(&a[48], &w[nw - 8]);
            cftmdl2(64, &a[64], &w[nw - 64]);
            cftf081(&a[64], &w[nw - 8]);
            cftf082(&a[80], &w[nw - 8]);
            cftf081(&a[96], &w[nw - 8]);
            cftf082(&a[112], &w[nw - 8]);
            cftmdl1(64, &a[128], &w[nw - 32]);
            cftf081(&a[128], &w[nw - 8]);
            cftf082(&a[144], &w[nw - 8]);
            cftf081(&a[160], &w[nw - 8]);
            cftf081(&a[176], &w[nw - 8]);
            if (isplt != 0) {
                cftmdl1(64, &a[192], &w[nw - 32]);
                cftf081(&a[240], &w[nw - 8]);
            }
            else {
                cftmdl2(64, &a[192], &w[nw - 64]);
                cftf082(&a[240], &w[nw - 8]);
            }
            cftf081(&a[192], &w[nw - 8]);
            cftf082(&a[208], &w[nw - 8]);
            cftf081(&a[224], &w[nw - 8]);
        }
    }

    static void cftrec4(int n, double* a, int nw, double* w)
    {
        int isplt, j, k, m;

        m = n;
        while (m > 512) {
            m >>= 2;
            cftmdl1(m, &a[n - m], &w[nw - (m >> 1)]);
        }
        cftleaf(m, 1, &a[n - m], nw, w);
        k = 0;
        for (j = n - m; j > 0; j -= m) {
            k++;
            isplt = cfttree(m, j, k, a, nw, w);
            cftleaf(m, isplt, &a[j - m], nw, w);
        }
    }

    static void rftfsub(int n, double* a, int nc, double* c)
    {
        int j, k, kk, ks, m;
        double wkr, wki, xr, xi, yr, yi;

        m = n >> 1;
        ks = 2 * nc / m;
        kk = 0;
        for (j = 2; j < m; j += 2) {
            k = n - j;
            kk += ks;
            wkr = 0.5 - c[nc - kk];
            wki = c[kk];
            xr = a[j] - a[k];
            xi = a[j + 1] + a[k + 1];
            yr = wkr * xr - wki * xi;
            yi = wkr * xi + wki * xr;
            a[j] -= yr;
            a[j + 1] -= yi;
            a[k] += yr;
            a[k + 1] -= yi;
        }
    }

    static void rftbsub(int n, double* a, int nc, double* c)
    {
        int j, k, kk, ks, m;
        double wkr, wki, xr, xi, yr, yi;

        m = n >> 1;
        ks = 2 * nc / m;
        kk = 0;
        for (j = 2; j < m; j += 2) {
            k = n - j;
            kk += ks;
            wkr = 0.5 - c[nc - kk];
            wki = c[kk];
            xr = a[j] - a[k];
            xi = a[j + 1] + a[k + 1];
            yr = wkr * xr + wki * xi;
            yi = wkr * xi - wki * xr;
            a[j] -= yr;
            a[j + 1] -= yi;
            a[k] += yr;
            a[k + 1] -= yi;
        }
    }

#ifdef USE_CDFT_THREADS
    struct cdft_arg_st {
        int n0;
        int n;
        double* a;
        int nw;
        double* w;
    };
    typedef struct cdft_arg_st cdft_arg_t;

    static void cftrec4_th(int n, double* a, int nw, double* w)
    {
        int i, idiv4, m, nthread;
        cdft_thread_t th[4];
        cdft_arg_t ag[4];

        nthread = 2;
        idiv4 = 0;
        m = n >> 1;
        if (n > CDFT_4THREADS_BEGIN_N) {
            nthread = 4;
            idiv4 = 1;
            m >>= 1;
        }
        for (i = 0; i < nthread; i++) {
            ag[i].n0 = n;
            ag[i].n = m;
            ag[i].a = &a[i * m];
            ag[i].nw = nw;
            ag[i].w = w;
            if (i != idiv4) {
                cdft_thread_create(&th[i], cftrec1_th, &ag[i]);
            }
            else {
                cdft_thread_create(&th[i], cftrec2_th, &ag[i]);
            }
        }
        for (i = 0; i < nthread; i++) {
            cdft_thread_wait(th[i]);
        }
    }

    static void* cftrec1_th(void* p)
    {
        int cfttree(int n, int j, int k, double* a, int nw, double* w);
        void cftleaf(int n, int isplt, double* a, int nw, double* w);
        void cftmdl1(int n, double* a, double* w);
        int isplt, j, k, m, n, n0, nw;
        double* a, * w;

        n0 = ((cdft_arg_t*)p)->n0;
        n = ((cdft_arg_t*)p)->n;
        a = ((cdft_arg_t*)p)->a;
        nw = ((cdft_arg_t*)p)->nw;
        w = ((cdft_arg_t*)p)->w;
        m = n0;
        while (m > 512) {
            m >>= 2;
            cftmdl1(m, &a[n - m], &w[nw - (m >> 1)]);
        }
        cftleaf(m, 1, &a[n - m], nw, w);
        k = 0;
        for (j = n - m; j > 0; j -= m) {
            k++;
            isplt = cfttree(m, j, k, a, nw, w);
            cftleaf(m, isplt, &a[j - m], nw, w);
        }
        return (void*)0;
    }

    static void* cftrec2_th(void* p)
    {
        int cfttree(int n, int j, int k, double* a, int nw, double* w);
        void cftleaf(int n, int isplt, double* a, int nw, double* w);
        void cftmdl2(int n, double* a, double* w);
        int isplt, j, k, m, n, n0, nw;
        double* a, * w;

        n0 = ((cdft_arg_t*)p)->n0;
        n = ((cdft_arg_t*)p)->n;
        a = ((cdft_arg_t*)p)->a;
        nw = ((cdft_arg_t*)p)->nw;
        w = ((cdft_arg_t*)p)->w;
        k = 1;
        m = n0;
        while (m > 512) {
            m >>= 2;
            k <<= 2;
            cftmdl2(m, &a[n - m], &w[nw - m]);
        }
        cftleaf(m, 0, &a[n - m], nw, w);
        k >>= 1;
        for (j = n - m; j > 0; j -= m) {
            k++;
            isplt = cfttree(m, j, k, a, nw, w);
            cftleaf(m, isplt, &a[j - m], nw, w);
        }
        return (void*)0;
    }
#endif /* USE_CDFT_THREADS */

    static void cftbsub(int n, double* a, int* ip, int nw, double* w)
    {
        if (n > 8) {
            if (n > 32) {
                cftb1st(n, a, &w[nw - (n >> 2)]);
#ifdef USE_CDFT_THREADS
                if (n > CDFT_THREADS_BEGIN_N) {
                    cftrec4_th(n, a, nw, w);
                }
                else
#endif /* USE_CDFT_THREADS */
                    if (n > 512) {
                        cftrec4(n, a, nw, w);
                    }
                    else if (n > 128) {
                        cftleaf(n, 1, a, nw, w);
                    }
                    else {
                        cftfx41(n, a, nw, w);
                    }
                bitrv2conj(n, ip, a);
            }
            else if (n == 32) {
                cftf161(a, &w[nw - 8]);
                bitrv216neg(a);
            }
            else {
                cftf081(a, w);
                bitrv208neg(a);
            }
        }
        else if (n == 8) {
            cftb040(a);
        }
        else if (n == 4) {
            cftx020(a);
        }
    }

    static void cftfsub(int n, double* a, int* ip, int nw, double* w)
    {
        if (n > 8) {
            if (n > 32) {
                cftf1st(n, a, &w[nw - (n >> 2)]);
#ifdef USE_CDFT_THREADS
                if (n > CDFT_THREADS_BEGIN_N) {
                    cftrec4_th(n, a, nw, w);
                }
                else
#endif /* USE_CDFT_THREADS */
                    if (n > 512) {
                        cftrec4(n, a, nw, w);
                    }
                    else if (n > 128) {
                        cftleaf(n, 1, a, nw, w);
                    }
                    else {
                        cftfx41(n, a, nw, w);
                    }
                bitrv2(n, ip, a);
            }
            else if (n == 32) {
                cftf161(a, &w[nw - 8]);
                bitrv216(a);
            }
            else {
                cftf081(a, w);
                bitrv208(a);
            }
        }
        else if (n == 8) {
            cftf040(a);
        }
        else if (n == 4) {
            cftx020(a);
        }
    }

    void rdft(int n, int isgn, double* a, int* ip, double* w)
    {
        int nw, nc;
        double xi;

        nw = ip[0];
        if (n > (nw << 2)) {
            nw = n >> 2;
            makewt(nw, ip, w);
        }
        nc = ip[1];
        if (n > (nc << 2)) {
            nc = n >> 2;
            makect(nc, ip, w + nw);
        }
        if (isgn >= 0) {
            if (n > 4) {
                cftfsub(n, a, ip, nw, w);
                rftfsub(n, a, nc, w + nw);
            }
            else if (n == 4) {
                cftfsub(n, a, ip, nw, w);
            }
            xi = a[0] - a[1];
            a[0] += a[1];
            a[1] = xi;
        }
        else {
            a[1] = 0.5 * (a[0] - a[1]);
            a[0] -= a[1];
            if (n > 4) {
                rftbsub(n, a, nc, w + nw);
                cftbsub(n, a, ip, nw, w);
            }
            else if (n == 4) {
                cftbsub(n, a, ip, nw, w);
            }
        }
    }


    inline int32_t RoundUpToNearestPowerOfTwo(int32_t n) {
        // copied from kaldi/src/base/kaldi-math.cc
        KNF_CHECK_GT(n, 0);
        n--;
        n |= n >> 1;
        n |= n >> 2;
        n |= n >> 4;
        n |= n >> 8;
        n |= n >> 16;
        return n + 1;
    }

    struct FrameExtractionOptions {
        float samp_freq = 16000;
        float frame_shift_ms = 10.0f;   // in milliseconds.
        float frame_length_ms = 25.0f;  // in milliseconds.

        float dither = 0.00003f;  // Amount of dithering, 0.0 means no dither.
        // Value 0.00003f is equivalent to 1.0 in kaldi.

        float preemph_coeff = 0.97f;        // Preemphasis coefficient.
        bool remove_dc_offset = true;       // Subtract mean of wave before FFT.
        std::string window_type = "povey";  // e.g. Hamming window
        // May be "hamming", "rectangular", "povey", "hanning", "hann", "sine",
        // "blackman".
        // "povey" is a window I made to be similar to Hamming but to go to zero at
        // the edges, it's pow((0.5 - 0.5*cos(n/N*2*pi)), 0.85) I just don't think the
        // Hamming window makes sense as a windowing function.
        bool round_to_power_of_two = true;
        float blackman_coeff = 0.42f;
        bool snip_edges = true;
        // bool allow_downsample = false;
        // bool allow_upsample = false;

        int32_t WindowShift() const {
            return static_cast<int32_t>(samp_freq * 0.001f * frame_shift_ms);
        }
        int32_t WindowSize() const {
            return static_cast<int32_t>(samp_freq * 0.001f * frame_length_ms);
        }
        int32_t PaddedWindowSize() const {
            return (round_to_power_of_two ? RoundUpToNearestPowerOfTwo(WindowSize())
                : WindowSize());
        }
        std::string ToString() const {
            std::ostringstream os;
#define KNF_PRINT(x) os << #x << ": " << x << "\n"
            KNF_PRINT(samp_freq);
            KNF_PRINT(frame_shift_ms);
            KNF_PRINT(frame_length_ms);
            KNF_PRINT(dither);
            KNF_PRINT(preemph_coeff);
            KNF_PRINT(remove_dc_offset);
            KNF_PRINT(window_type);
            KNF_PRINT(round_to_power_of_two);
            KNF_PRINT(blackman_coeff);
            KNF_PRINT(snip_edges);
            // KNF_PRINT(allow_downsample);
            // KNF_PRINT(allow_upsample);
#undef KNF_PRINT
            return os.str();
        }
    };

    std::ostream& operator<<(std::ostream& os, const FrameExtractionOptions& opts);

    class FeatureWindowFunction {
    public:
        FeatureWindowFunction() = default;
        explicit FeatureWindowFunction(const FrameExtractionOptions& opts);
        FeatureWindowFunction(const std::string& window_type, int32_t window_size,
            float blackman_coeff = 0.42);

        explicit FeatureWindowFunction(const std::vector<float>& window);

        /**
         * @param wave Pointer to a 1-D array of shape [window_size].
         *             It is modified in-place: wave[i] = wave[i] * window_[i].
         * @param
         */
        void Apply(float* wave) const;

        const std::vector<float>& GetWindow() const { return window_; }

    private:
        std::vector<float> window_;  // of size opts.WindowSize()
    };

    int64_t FirstSampleOfFrame(int32_t frame, const FrameExtractionOptions& opts);

    /**
       This function returns the number of frames that we can extract from a wave
       file with the given number of samples in it (assumed to have the same
       sampling rate as specified in 'opts').

          @param [in] num_samples  The number of samples in the wave file.
          @param [in] opts     The frame-extraction options class

          @param [in] flush   True if we are asserting that this number of samples
       is 'all there is', false if we expecting more data to possibly come in.  This
       only makes a difference to the answer
       if opts.snip_edges== false.  For offline feature extraction you always want
       flush == true.  In an online-decoding context, once you know (or decide) that
       no more data is coming in, you'd call it with flush == true at the end to
       flush out any remaining data.
    */
    int32_t NumFrames(int64_t num_samples, const FrameExtractionOptions& opts,
        bool flush = true);

    /*
      ExtractWindow() extracts a windowed frame of waveform (possibly with a
      power-of-two, padded size, depending on the config), including all the
      processing done by ProcessWindow().

      @param [in] sample_offset  If 'wave' is not the entire waveform, but
                       part of it to the left has been discarded, then the
                       number of samples prior to 'wave' that we have
                       already discarded.  Set this to zero if you are
                       processing the entire waveform in one piece, or
                       if you get 'no matching function' compilation
                       errors when updating the code.
      @param [in] wave  The waveform
      @param [in] f     The frame index to be extracted, with
                        0 <= f < NumFrames(sample_offset + wave.Dim(), opts, true)
      @param [in] opts  The options class to be used
      @param [in] window_function  The windowing function, as derived from the
                        options class.
      @param [out] window  The windowed, possibly-padded waveform to be
                         extracted.  Will be resized as needed.
      @param [out] log_energy_pre_window  If non-NULL, the log-energy of
                       the signal prior to pre-emphasis and multiplying by
                       the windowing function will be written to here.
    */
    void ExtractWindow(int64_t sample_offset, const std::vector<float>& wave,
        int32_t f, const FrameExtractionOptions& opts,
        const FeatureWindowFunction& window_function,
        std::vector<float>* window,
        float* log_energy_pre_window = nullptr);

    /**
      This function does all the windowing steps after actually
      extracting the windowed signal: depending on the
      configuration, it does dithering, dc offset removal,
      preemphasis, and multiplication by the windowing function.
       @param [in] opts  The options class to be used
       @param [in] window_function  The windowing function-- should have
                        been initialized using 'opts'.
       @param [in,out] window  A vector of size opts.WindowSize().  Note:
          it will typically be a sub-vector of a larger vector of size
          opts.PaddedWindowSize(), with the remaining samples zero,
          as the FFT code is more efficient if it operates on data with
          power-of-two size.
       @param [out]   log_energy_pre_window If non-NULL, then after dithering and
          DC offset removal, this function will write to this pointer the log of
          the total energy (i.e. sum-squared) of the frame.
     */
    void ProcessWindow(const FrameExtractionOptions& opts,
        const FeatureWindowFunction& window_function, float* window,
        float* log_energy_pre_window = nullptr);

    // Compute the inner product of two vectors
    float InnerProduct(const float* a, const float* b, int32_t n);

    std::vector<float> GetWindow(const std::string& window_type,
        int32_t window_size, float blackman_coeff = 0.42);


    std::ostream& operator<<(std::ostream& os, const FrameExtractionOptions& opts) {
        os << opts.ToString();
        return os;
    }

    std::vector<float> GetWindow(const std::string& window_type,
        int32_t window_size,
        float blackman_coeff /*= 0.42*/) {
        std::vector<float> window(window_size);
        int32_t frame_length = window_size;
        KNF_CHECK_GT(frame_length, 0);

        float* window_data = window.data();

        double a = M_2PI / (frame_length - 1);
        if (window_type == "hann") {
            // see https://pytorch.org/docs/stable/generated/torch.hann_window.html
            // We assume periodic is true
            a = M_2PI / frame_length;
        }

        for (int32_t i = 0; i < frame_length; i++) {
            double i_fl = static_cast<double>(i);
            if (window_type == "hanning") {
                window_data[i] = 0.5 - 0.5 * cos(a * i_fl);
            }
            else if (window_type == "sine") {
                // when you are checking ws wikipedia, please
                // note that 0.5 * a = M_PI/(frame_length-1)
                window_data[i] = sin(0.5 * a * i_fl);
            }
            else if (window_type == "hamming") {
                window_data[i] = 0.54 - 0.46 * cos(a * i_fl);
            }
            else if (window_type == "hann") {
                window_data[i] = 0.50 - 0.50 * cos(a * i_fl);
            }
            else if (window_type == "povey") {
                // like hamming but goes to zero at edges.
                window_data[i] = pow(0.5 - 0.5 * cos(a * i_fl), 0.85);
            }
            else if (window_type == "rectangular") {
                window_data[i] = 1.0;
            }
            else if (window_type == "blackman") {
                window_data[i] = blackman_coeff - 0.5 * cos(a * i_fl) +
                    (0.5 - blackman_coeff) * cos(2 * a * i_fl);
            }
            else {
                fprintf(stderr, "Invalid window type '%s'\n", window_type.c_str());
                exit(-1);
            }
        }

        return window;
    }

    FeatureWindowFunction::FeatureWindowFunction(const FrameExtractionOptions& opts)
        : FeatureWindowFunction(opts.window_type, opts.WindowSize(),
            opts.blackman_coeff) {
    }

    FeatureWindowFunction::FeatureWindowFunction(const std::string& window_type,
        int32_t window_size,
        float blackman_coeff /*= 0.42*/)
        : window_(knf::GetWindow(window_type, window_size, blackman_coeff)) {
    }

    FeatureWindowFunction::FeatureWindowFunction(const std::vector<float>& window)
        : window_(window) {
    }

    void FeatureWindowFunction::Apply(float* wave) const {
        int32_t window_size = window_.size();
        const float* p = window_.data();
        for (int32_t k = 0; k != window_size; ++k) {
            wave[k] *= p[k];
        }
    }

    int64_t FirstSampleOfFrame(int32_t frame, const FrameExtractionOptions& opts) {
        int64_t frame_shift = opts.WindowShift();
        if (opts.snip_edges) {
            return frame * frame_shift;
        }
        else {
            int64_t midpoint_of_frame = frame_shift * frame + frame_shift / 2,
                beginning_of_frame = midpoint_of_frame - opts.WindowSize() / 2;
            return beginning_of_frame;
        }
    }

    int32_t NumFrames(int64_t num_samples, const FrameExtractionOptions& opts,
        bool flush /*= true*/) {
        int64_t frame_shift = opts.WindowShift();
        int64_t frame_length = opts.WindowSize();
        if (opts.snip_edges) {
            // with --snip-edges=true (the default), we use a HTK-like approach to
            // determining the number of frames-- all frames have to fit completely into
            // the waveform, and the first frame begins at sample zero.
            if (num_samples < frame_length)
                return 0;
            else
                return (1 + ((num_samples - frame_length) / frame_shift));
            // You can understand the expression above as follows: 'num_samples -
            // frame_length' is how much room we have to shift the frame within the
            // waveform; 'frame_shift' is how much we shift it each time; and the ratio
            // is how many times we can shift it (integer arithmetic rounds down).
        }
        else {
            // if --snip-edges=false, the number of frames is determined by rounding the
            // (file-length / frame-shift) to the nearest integer.  The point of this
            // formula is to make the number of frames an obvious and predictable
            // function of the frame shift and signal length, which makes many
            // segmentation-related questions simpler.
            //
            // Because integer division in C++ rounds toward zero, we add (half the
            // frame-shift minus epsilon) before dividing, to have the effect of
            // rounding towards the closest integer.
            int32_t num_frames = (num_samples + (frame_shift / 2)) / frame_shift;

            if (flush) return num_frames;

            // note: 'end' always means the last plus one, i.e. one past the last.
            int64_t end_sample_of_last_frame =
                FirstSampleOfFrame(num_frames - 1, opts) + frame_length;

            // the following code is optimized more for clarity than efficiency.
            // If flush == false, we can't output frames that extend past the end
            // of the signal.
            while (num_frames > 0 && end_sample_of_last_frame > num_samples) {
                num_frames--;
                end_sample_of_last_frame -= frame_shift;
            }
            return num_frames;
        }
    }

    void ExtractWindow(int64_t sample_offset, const std::vector<float>& wave,
        int32_t f, const FrameExtractionOptions& opts,
        const FeatureWindowFunction& window_function,
        std::vector<float>* window,
        float* log_energy_pre_window /*= nullptr*/) {
        KNF_CHECK(sample_offset >= 0 && wave.size() != 0);

        int32_t frame_length = opts.WindowSize();
        int32_t frame_length_padded = opts.PaddedWindowSize();

        int64_t num_samples = sample_offset + wave.size();
        int64_t start_sample = FirstSampleOfFrame(f, opts);
        int64_t end_sample = start_sample + frame_length;

        if (opts.snip_edges) {
            KNF_CHECK(start_sample >= sample_offset && end_sample <= num_samples);
        }
        else {
            KNF_CHECK(sample_offset == 0 || start_sample >= sample_offset);
        }

        if (window->size() != frame_length_padded) {
            window->resize(frame_length_padded);
        }

        // wave_start and wave_end are start and end indexes into 'wave', for the
        // piece of wave that we're trying to extract.
        int32_t wave_start = int32_t(start_sample - sample_offset);
        int32_t wave_end = wave_start + frame_length;

        if (wave_start >= 0 && wave_end <= wave.size()) {
            // the normal case-- no edge effects to consider.
            std::copy(wave.begin() + wave_start,
                wave.begin() + wave_start + frame_length, window->data());
        }
        else {
            // Deal with any end effects by reflection, if needed.  This code will only
            // be reached for about two frames per utterance, so we don't concern
            // ourselves excessively with efficiency.
            int32_t wave_dim = wave.size();
            for (int32_t s = 0; s < frame_length; ++s) {
                int32_t s_in_wave = s + wave_start;
                while (s_in_wave < 0 || s_in_wave >= wave_dim) {
                    // reflect around the beginning or end of the wave.
                    // e.g. -1 -> 0, -2 -> 1.
                    // dim -> dim - 1, dim + 1 -> dim - 2.
                    // the code supports repeated reflections, although this
                    // would only be needed in pathological cases.
                    if (s_in_wave < 0)
                        s_in_wave = -s_in_wave - 1;
                    else
                        s_in_wave = 2 * wave_dim - 1 - s_in_wave;
                }
                (*window)[s] = wave[s_in_wave];
            }
        }

        ProcessWindow(opts, window_function, window->data(), log_energy_pre_window);
    }

    static void RemoveDcOffset(float* d, int32_t n) {
        float sum = 0;
        for (int32_t i = 0; i != n; ++i) {
            sum += d[i];
        }

        float mean = sum / n;

        for (int32_t i = 0; i != n; ++i) {
            d[i] -= mean;
        }
    }

    float InnerProduct(const float* a, const float* b, int32_t n) {
        float sum = 0;
        for (int32_t i = 0; i != n; ++i) {
            sum += a[i] * b[i];
        }
        return sum;
    }

    void Dither(float* d, int32_t n, float dither_value) {
        if (dither_value == 0.0) {
            return;
        }

        RandomState rstate;
        for (int32_t i = 0; i < n; ++i) {
            d[i] += RandGauss(&rstate) * dither_value;
        }
    }

    static void Preemphasize(float* d, int32_t n, float preemph_coeff) {
        if (preemph_coeff == 0.0) {
            return;
        }

        KNF_CHECK(preemph_coeff >= 0.0 && preemph_coeff <= 1.0);

        for (int32_t i = n - 1; i > 0; --i) {
            d[i] -= preemph_coeff * d[i - 1];
        }
        d[0] -= preemph_coeff * d[0];
    }

    void ProcessWindow(const FrameExtractionOptions& opts,
        const FeatureWindowFunction& window_function, float* window,
        float* log_energy_pre_window /*= nullptr*/) {
        int32_t frame_length = opts.WindowSize();

        if (opts.dither != 0.0) {
            Dither(window, frame_length, opts.dither);
        }

        if (opts.remove_dc_offset) {
            RemoveDcOffset(window, frame_length);
        }

        if (log_energy_pre_window != NULL) {
            float energy = std::max<float>(InnerProduct(window, window, frame_length),
                std::numeric_limits<float>::epsilon());
            *log_energy_pre_window = std::log(energy);
        }

        if (opts.preemph_coeff != 0.0) {
            Preemphasize(window, frame_length, opts.preemph_coeff);
        }

        window_function.Apply(window);
    }


    std::string StftConfig::ToString() const {
        std::ostringstream os;
        os << "StftConfig(";
        os << "n_fft=" << n_fft << ", ";
        os << "hop_length=" << hop_length << ", ";
        os << "win_length=" << win_length << ", ";
        os << "window_type=\"" << window_type << "\", ";
        os << "center=" << (center ? "True" : "False") << ", ";
        os << "pad_mode=\"" << pad_mode << "\", ";
        os << "normalized=" << (normalized ? "True" : "False") << ")";
        return os.str();
    }

    class Stft::Impl {
    public:
        explicit Impl(const StftConfig& config) : config_(config) {
            if (!config.window.empty()) {
                window_ = std::make_unique<FeatureWindowFunction>(config.window);
            }
            else if (!config.window_type.empty()) {
                window_ = std::make_unique<FeatureWindowFunction>(config.window_type,
                    config.win_length);
            }
        }

        StftResult Compute(const float* data, int32_t n) const {
            int32_t n_fft = config_.n_fft;
            int32_t hop_length = config_.hop_length;

            std::vector<float> samples;
            const float* p = data;

            if (config_.center) {
                samples = Pad(data, n);
                p = samples.data();
                n = samples.size();
            }

            int64_t num_frames = 1 + (n - n_fft) / hop_length;

            Rfft rfft(n_fft);

            StftResult ans;
            ans.num_frames = num_frames;
            ans.real.resize(num_frames * (n_fft / 2 + 1));
            ans.imag.resize(num_frames * (n_fft / 2 + 1));

            std::vector<float> tmp(config_.n_fft);

            for (int32_t i = 0; i < num_frames; ++i) {
                tmp = { p + i * hop_length, p + i * hop_length + n_fft };
                if (window_) {
                    window_->Apply(tmp.data());
                }

                rfft.Compute(tmp.data());

                for (int32_t k = 0; k < n_fft / 2; ++k) {
                    if (k == 0) {
                        ans.real[i * (n_fft / 2 + 1)] = tmp[0];
                        ans.real[i * (n_fft / 2 + 1) + n_fft / 2] = tmp[1];
                    }
                    else {
                        ans.real[i * (n_fft / 2 + 1) + k] = tmp[2 * k];

                        // we use -1 here so it matches the results of torch.stft
                        ans.imag[i * (n_fft / 2 + 1) + k] = -1 * tmp[2 * k + 1];
                    }
                }
            }

            if (config_.normalized) {
                float scale = 1 / std::sqrt(n_fft);
                for (int32_t i = 0; i < ans.real.size(); ++i) {
                    ans.real[i] *= scale;
                    ans.imag[i] *= scale;
                }
            }

            return ans;
        }

        std::vector<float> Pad(const float* data, int32_t n) const {
            int32_t pad_amount = config_.n_fft / 2;
            std::vector<float> ans(n + config_.n_fft);
            std::copy(data, data + n, ans.begin() + pad_amount);
            if (config_.pad_mode == "constant") {
                // do nothing
            }
            else if (config_.pad_mode == "reflect") {
                // left
                std::copy(data + 1, data + 1 + pad_amount, ans.rend() - pad_amount);
                std::copy(data + n - pad_amount - 1, data + n - 1, ans.rbegin());
            }
            else if (config_.pad_mode == "replicate") {
                std::fill(ans.begin(), ans.begin() + pad_amount, data[0]);
                std::fill(ans.end() - pad_amount, ans.end(), data[n - 1]);
            }
            else {
                fprintf(stderr, "Unsupported pad_mode: '%s'. Use 0 padding\n",
                    config_.pad_mode.c_str());
            }

            return ans;
        }

    private:
        StftConfig config_;
        std::unique_ptr<FeatureWindowFunction> window_;
    };

    Stft::Stft(const StftConfig& config) : impl_(std::make_unique<Impl>(config)) {}

    Stft::~Stft() = default;

    StftResult Stft::Compute(const float* data, int32_t n) const {
        return impl_->Compute(data, n);
    }

class IStft::Impl {
 public:
  explicit Impl(const StftConfig &config) : config_(config) {
    if (!config.window.empty()) {
      window_ = std::make_unique<FeatureWindowFunction>(config_.window);
    } else if (!config.window_type.empty()) {
      window_ = std::make_unique<FeatureWindowFunction>(config.window_type,
                                                        config.win_length);
    }
  }

  std::vector<float> Compute(const StftResult &stft_result) const {
    Rfft rfft(config_.n_fft, true);

    int32_t num_samples =
        config_.n_fft + (stft_result.num_frames - 1) * config_.hop_length;

    std::vector<float> samples(num_samples);
    for (int32_t i = 0; i < stft_result.num_frames; ++i) {
      auto x = InverseFFT(stft_result, i, &rfft);
      OverlapAdd(std::move(x), i, &samples);
    }

    auto denominator = GetDenominator(stft_result.num_frames);

    for (int32_t i = 0; i < num_samples; ++i) {
      if (denominator[i]) {
        samples[i] /= denominator[i];
      }
    }

    if (config_.center) {
      samples = {samples.begin() + config_.n_fft / 2,
                 samples.end() - config_.n_fft / 2};
    }

    return samples;
  }

  std::vector<float> InverseFFT(const StftResult &r, int32_t frame_index,
                                Rfft *rfft) const {
    int32_t n_fft = config_.n_fft;
    int32_t hop_length = config_.hop_length;

    const float *p_real = r.real.data() + frame_index * (n_fft / 2 + 1);
    const float *p_imag = r.imag.data() + frame_index * (n_fft / 2 + 1);
    std::vector<float> tmp(n_fft);

    float scale = 1;
    if (config_.normalized) {
      scale = std::sqrt(n_fft);
    }

    for (int32_t i = 0; i < n_fft / 2; ++i) {
      if (i == 0) {
        tmp[0] = p_real[0] * scale;
        tmp[1] = p_real[n_fft / 2] * scale;
      } else {
        tmp[2 * i] = p_real[i] * scale;
        tmp[2 * i + 1] = -1 * p_imag[i] * scale;
      }
    }

    rfft->Compute(tmp.data());

    scale = 2.0f / n_fft;
    for (auto &f : tmp) {
      f *= scale;
    }

    return tmp;
  }

  void OverlapAdd(std::vector<float> current_frame, int32_t frame_index,
                  std::vector<float> *samples) const {
    if (window_) {
      window_->Apply(current_frame.data());
    }

    float *p = samples->data() + frame_index * config_.hop_length;
    for (int32_t i = 0; i < config_.n_fft; ++i) {
      p[i] += current_frame[i];
    }
  }

  std::vector<float> GetDenominator(int32_t num_frames) const {
    int32_t num_samples = config_.n_fft + (num_frames - 1) * config_.hop_length;
    std::vector<float> ans(num_samples);
    if (!window_) {
      for (int32_t i = 0; i < num_frames; ++i) {
        int32_t start = i * config_.hop_length;
        int32_t end = start + config_.n_fft;

        for (int32_t k = start; k < end; ++k) {
          ans[k] += 1;
        }
      }
    } else {
      const auto &w = window_->GetWindow();
      for (int32_t i = 0; i < num_frames; ++i) {
        int32_t start = i * config_.hop_length;
        int32_t end = start + config_.n_fft;

        auto pw = w.data();
        for (int32_t k = start; k < end; ++k, ++pw) {
          ans[k] += (*pw) * (*pw);
        }
      }
    }

    return ans;
  }

 private:
  StftConfig config_;
  std::unique_ptr<FeatureWindowFunction> window_;
};

IStft::IStft(const StftConfig &config)
    : impl_(std::make_unique<Impl>(config)) {}

IStft::~IStft() = default;

std::vector<float> IStft::Compute(const StftResult &stft_result) const {
  return impl_->Compute(stft_result);
}

}  // namespace knf
