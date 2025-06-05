#ifndef ECC_UTILS_H
#define ECC_UTILS_H

#include <string>
#include <vector>
#include <gmp.h>

// Big integer type (using GMP)
struct BigInt {
    mpz_t value;
    
    BigInt() { mpz_init(value); }
    BigInt(const BigInt& other) { mpz_init_set(value, other.value); }
    BigInt(BigInt&& other) { mpz_init(value); mpz_swap(value, other.value); }
    ~BigInt() { mpz_clear(value); }
    
    BigInt& operator=(const BigInt& other) {
        if (this != &other) {
            mpz_set(value, other.value);
        }
        return *this;
    }
    
    BigInt& operator=(BigInt&& other) {
        if (this != &other) {
            mpz_swap(value, other.value);
        }
        return *this;
    }
};

// Elliptic curve point
struct ECPoint {
    BigInt x, y;
    bool is_infinity;
    
    ECPoint() : is_infinity(false) {}
    ECPoint(const BigInt& x_coord, const BigInt& y_coord) : x(x_coord), y(y_coord), is_infinity(false) {}
};

// secp256k1 parameters
extern const BigInt SECP256K1_P;    // Field prime
extern const BigInt SECP256K1_N;    // Order of the curve
extern const BigInt SECP256K1_A;    // Curve parameter a (0)
extern const BigInt SECP256K1_B;    // Curve parameter b (7)
extern const ECPoint SECP256K1_G;   // Generator point

// Basic arithmetic operations
BigInt bigint_zero();
BigInt bigint_one();
BigInt bigint_from_int(int value);
BigInt bigint_from_uint64(uint64_t value);
BigInt bigint_add(const BigInt& a, const BigInt& b);
BigInt bigint_subtract(const BigInt& a, const BigInt& b);
BigInt bigint_multiply(const BigInt& a, const BigInt& b);
BigInt bigint_mod(const BigInt& a, const BigInt& modulus);
BigInt bigint_mod_inverse(const BigInt& a, const BigInt& modulus);
BigInt bigint_shift_left(const BigInt& a, int bits);
BigInt bigint_shift_right(const BigInt& a, int bits);
int bigint_compare(const BigInt& a, const BigInt& b);
int bigint_bit_length(const BigInt& a);

// Conversion functions
std::string bigint_to_hex(const BigInt& a);
BigInt hex_to_bigint(const std::string& hex);
std::string bigint_to_string(const BigInt& a);

// Elliptic curve operations
ECPoint point_add(const ECPoint& p1, const ECPoint& p2);
ECPoint point_double(const ECPoint& p);
ECPoint point_multiply(const BigInt& k, const ECPoint& p);
bool point_equals(const ECPoint& p1, const ECPoint& p2);
bool point_is_on_curve(const ECPoint& p);

// Utility functions
ECPoint get_generator();
BigInt get_field_prime();
BigInt get_curve_order();
bool hex_to_point(const std::string& hex, ECPoint& point);
std::string point_to_hex(const ECPoint& point);

// Hash functions
std::string sha256(const std::string& input);
std::string hash160(const std::string& input);

// Bitcoin address functions
std::string pubkey_to_address(const ECPoint& pubkey, bool compressed = true);
bool is_valid_address(const std::string& address);

#endif // ECC_UTILS_H
