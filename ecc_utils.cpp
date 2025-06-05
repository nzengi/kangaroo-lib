#include "ecc_utils.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <openssl/sha.h>
#include <openssl/ripemd.h>

// secp256k1 curve parameters
static BigInt create_secp256k1_p() {
    BigInt p;
    mpz_set_str(p.value, "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F", 16);
    return p;
}

static BigInt create_secp256k1_n() {
    BigInt n;
    mpz_set_str(n.value, "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141", 16);
    return n;
}

static BigInt create_secp256k1_a() {
    return bigint_zero();
}

static BigInt create_secp256k1_b() {
    return bigint_from_int(7);
}

static ECPoint create_secp256k1_g() {
    BigInt gx, gy;
    mpz_set_str(gx.value, "79BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798", 16);
    mpz_set_str(gy.value, "483ADA7726A3C4655DA4FBFC0E1108A8FD17B448A68554199C47D08FFB10D4B8", 16);
    return ECPoint(gx, gy);
}

const BigInt SECP256K1_P = create_secp256k1_p();
const BigInt SECP256K1_N = create_secp256k1_n();
const BigInt SECP256K1_A = create_secp256k1_a();
const BigInt SECP256K1_B = create_secp256k1_b();
const ECPoint SECP256K1_G = create_secp256k1_g();

// Basic arithmetic operations
BigInt bigint_zero() {
    BigInt result;
    mpz_set_ui(result.value, 0);
    return result;
}

BigInt bigint_one() {
    BigInt result;
    mpz_set_ui(result.value, 1);
    return result;
}

BigInt bigint_from_int(int value) {
    BigInt result;
    mpz_set_si(result.value, value);
    return result;
}

BigInt bigint_from_uint64(uint64_t value) {
    BigInt result;
    mpz_set_ui(result.value, value);
    return result;
}

BigInt bigint_add(const BigInt& a, const BigInt& b) {
    BigInt result;
    mpz_add(result.value, a.value, b.value);
    return result;
}

BigInt bigint_subtract(const BigInt& a, const BigInt& b) {
    BigInt result;
    mpz_sub(result.value, a.value, b.value);
    return result;
}

BigInt bigint_multiply(const BigInt& a, const BigInt& b) {
    BigInt result;
    mpz_mul(result.value, a.value, b.value);
    return result;
}

BigInt bigint_mod(const BigInt& a, const BigInt& modulus) {
    BigInt result;
    mpz_mod(result.value, a.value, modulus.value);
    return result;
}

BigInt bigint_mod_inverse(const BigInt& a, const BigInt& modulus) {
    BigInt result;
    if (mpz_invert(result.value, a.value, modulus.value) == 0) {
        // No inverse exists
        mpz_set_ui(result.value, 0);
    }
    return result;
}

BigInt bigint_shift_left(const BigInt& a, int bits) {
    BigInt result;
    mpz_mul_2exp(result.value, a.value, bits);
    return result;
}

BigInt bigint_shift_right(const BigInt& a, int bits) {
    BigInt result;
    mpz_fdiv_q_2exp(result.value, a.value, bits);
    return result;
}

int bigint_compare(const BigInt& a, const BigInt& b) {
    return mpz_cmp(a.value, b.value);
}

int bigint_bit_length(const BigInt& a) {
    return mpz_sizeinbase(a.value, 2);
}

// Conversion functions
std::string bigint_to_hex(const BigInt& a) {
    char* hex_str = mpz_get_str(nullptr, 16, a.value);
    std::string result(hex_str);
    free(hex_str);
    
    // Convert to uppercase
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    
    return result;
}

BigInt hex_to_bigint(const std::string& hex) {
    BigInt result;
    std::string clean_hex = hex;
    
    // Remove 0x prefix if present
    if (clean_hex.length() >= 2 && clean_hex.substr(0, 2) == "0x") {
        clean_hex = clean_hex.substr(2);
    }
    
    mpz_set_str(result.value, clean_hex.c_str(), 16);
    return result;
}

std::string bigint_to_string(const BigInt& a) {
    char* str = mpz_get_str(nullptr, 10, a.value);
    std::string result(str);
    free(str);
    return result;
}

// Elliptic curve operations
ECPoint point_add(const ECPoint& p1, const ECPoint& p2) {
    if (p1.is_infinity) return p2;
    if (p2.is_infinity) return p1;
    
    // Check if points are the same
    if (bigint_compare(p1.x, p2.x) == 0) {
        if (bigint_compare(p1.y, p2.y) == 0) {
            return point_double(p1);
        } else {
            // Points are additive inverses
            ECPoint result;
            result.is_infinity = true;
            return result;
        }
    }
    
    // Calculate slope: s = (y2 - y1) / (x2 - x1)
    BigInt dy = bigint_subtract(p2.y, p1.y);
    BigInt dx = bigint_subtract(p2.x, p1.x);
    BigInt dx_inv = bigint_mod_inverse(dx, SECP256K1_P);
    BigInt s = bigint_mod(bigint_multiply(dy, dx_inv), SECP256K1_P);
    
    // Calculate result point
    BigInt s_squared = bigint_mod(bigint_multiply(s, s), SECP256K1_P);
    BigInt x3 = bigint_mod(bigint_subtract(bigint_subtract(s_squared, p1.x), p2.x), SECP256K1_P);
    BigInt y3 = bigint_mod(bigint_subtract(bigint_multiply(s, bigint_subtract(p1.x, x3)), p1.y), SECP256K1_P);
    
    return ECPoint(x3, y3);
}

ECPoint point_double(const ECPoint& p) {
    if (p.is_infinity) return p;
    
    // Check if y = 0 (point at infinity)
    if (bigint_compare(p.y, bigint_zero()) == 0) {
        ECPoint result;
        result.is_infinity = true;
        return result;
    }
    
    // Calculate slope: s = (3 * x^2 + a) / (2 * y)
    BigInt three = bigint_from_int(3);
    BigInt two = bigint_from_int(2);
    BigInt x_squared = bigint_mod(bigint_multiply(p.x, p.x), SECP256K1_P);
    BigInt numerator = bigint_mod(bigint_multiply(three, x_squared), SECP256K1_P); // a = 0 for secp256k1
    BigInt denominator = bigint_mod(bigint_multiply(two, p.y), SECP256K1_P);
    BigInt denom_inv = bigint_mod_inverse(denominator, SECP256K1_P);
    BigInt s = bigint_mod(bigint_multiply(numerator, denom_inv), SECP256K1_P);
    
    // Calculate result point
    BigInt s_squared = bigint_mod(bigint_multiply(s, s), SECP256K1_P);
    BigInt two_x = bigint_mod(bigint_multiply(two, p.x), SECP256K1_P);
    BigInt x3 = bigint_mod(bigint_subtract(s_squared, two_x), SECP256K1_P);
    BigInt y3 = bigint_mod(bigint_subtract(bigint_multiply(s, bigint_subtract(p.x, x3)), p.y), SECP256K1_P);
    
    return ECPoint(x3, y3);
}

ECPoint point_multiply(const BigInt& k, const ECPoint& p) {
    if (p.is_infinity) return p;
    
    ECPoint result;
    result.is_infinity = true;
    
    ECPoint addend = p;
    BigInt k_copy = k;
    
    while (bigint_compare(k_copy, bigint_zero()) > 0) {
        if (mpz_odd_p(k_copy.value)) {
            result = point_add(result, addend);
        }
        addend = point_double(addend);
        k_copy = bigint_shift_right(k_copy, 1);
    }
    
    return result;
}

bool point_equals(const ECPoint& p1, const ECPoint& p2) {
    if (p1.is_infinity && p2.is_infinity) return true;
    if (p1.is_infinity || p2.is_infinity) return false;
    
    return (bigint_compare(p1.x, p2.x) == 0) && (bigint_compare(p1.y, p2.y) == 0);
}

bool point_is_on_curve(const ECPoint& p) {
    if (p.is_infinity) return true;
    
    // Check if y^2 = x^3 + 7 (mod p)
    BigInt y_squared = bigint_mod(bigint_multiply(p.y, p.y), SECP256K1_P);
    BigInt x_cubed = bigint_mod(bigint_multiply(bigint_multiply(p.x, p.x), p.x), SECP256K1_P);
    BigInt right_side = bigint_mod(bigint_add(x_cubed, SECP256K1_B), SECP256K1_P);
    
    return bigint_compare(y_squared, right_side) == 0;
}

// Utility functions
ECPoint get_generator() {
    return SECP256K1_G;
}

BigInt get_field_prime() {
    return SECP256K1_P;
}

BigInt get_curve_order() {
    return SECP256K1_N;
}

bool hex_to_point(const std::string& hex, ECPoint& point) {
    try {
        if (hex.length() < 2) return false;
        
        std::string clean_hex = hex;
        if (clean_hex.substr(0, 2) == "0x") {
            clean_hex = clean_hex.substr(2);
        }
        
        if (clean_hex.length() == 66 && clean_hex[0] == '0') {
            // Compressed format
            bool is_even = (clean_hex[1] == '2');
            std::string x_hex = clean_hex.substr(2);
            
            point.x = hex_to_bigint(x_hex);
            
            // Calculate y coordinate
            BigInt x_cubed = bigint_mod(bigint_multiply(bigint_multiply(point.x, point.x), point.x), SECP256K1_P);
            BigInt y_squared = bigint_mod(bigint_add(x_cubed, SECP256K1_B), SECP256K1_P);
            
            // This is a simplified version - in practice, you'd need to implement modular square root
            // For now, we'll assume the input is valid and set a placeholder
            point.y = bigint_from_int(1); // Placeholder
            point.is_infinity = false;
            
            return true;
        } else if (clean_hex.length() == 128) {
            // Uncompressed format
            std::string x_hex = clean_hex.substr(0, 64);
            std::string y_hex = clean_hex.substr(64);
            
            point.x = hex_to_bigint(x_hex);
            point.y = hex_to_bigint(y_hex);
            point.is_infinity = false;
            
            return point_is_on_curve(point);
        }
        
        return false;
    } catch (...) {
        return false;
    }
}

std::string point_to_hex(const ECPoint& point) {
    if (point.is_infinity) {
        return "00";
    }
    
    // Return uncompressed format
    std::string x_hex = bigint_to_hex(point.x);
    std::string y_hex = bigint_to_hex(point.y);
    
    // Pad to 64 characters
    while (x_hex.length() < 64) x_hex = "0" + x_hex;
    while (y_hex.length() < 64) y_hex = "0" + y_hex;
    
    return "04" + x_hex + y_hex;
}

// Hash functions
std::string sha256(const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, input.c_str(), input.length());
    SHA256_Final(hash, &sha256);
    
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

std::string hash160(const std::string& input) {
    // First SHA256
    std::string sha256_result = sha256(input);
    
    // Convert hex to bytes
    std::vector<unsigned char> bytes;
    for (size_t i = 0; i < sha256_result.length(); i += 2) {
        std::string byte_str = sha256_result.substr(i, 2);
        bytes.push_back(std::stoi(byte_str, nullptr, 16));
    }
    
    // Then RIPEMD160
    unsigned char hash[RIPEMD160_DIGEST_LENGTH];
    RIPEMD160_CTX ripemd160;
    RIPEMD160_Init(&ripemd160);
    RIPEMD160_Update(&ripemd160, bytes.data(), bytes.size());
    RIPEMD160_Final(hash, &ripemd160);
    
    std::stringstream ss;
    for (int i = 0; i < RIPEMD160_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

// Bitcoin address functions (simplified)
std::string pubkey_to_address(const ECPoint& pubkey, bool compressed) {
    // This is a simplified version
    // In practice, you'd need proper Base58Check encoding
    std::string pubkey_hex = point_to_hex(pubkey);
    std::string hash160_result = hash160(pubkey_hex);
    return "1" + hash160_result.substr(0, 8) + "..."; // Placeholder
}

bool is_valid_address(const std::string& address) {
    // Simple validation - starts with 1, 3, or bc1
    return !address.empty() && (address[0] == '1' || address[0] == '3' || address.substr(0, 3) == "bc1");
}
