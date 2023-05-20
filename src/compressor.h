// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Copyright (c) 2021-2023 The MVC developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MVC_COMPRESSOR_H
#define MVC_COMPRESSOR_H

#include "consensus/consensus.h"
#include "primitives/transaction.h"
#include "serialize.h"

class CKeyID;
class CPubKey;
class CScriptID;

/**
 * Compact serializer for scripts.
 *
 * It detects common cases and encodes them much more efficiently.
 * 3 special cases are defined:
 *  * Pay to pubkey hash (encoded as 21 bytes)
 *  * Pay to script hash (encoded as 21 bytes)
 *  * Pay to pubkey starting with 0x02, 0x03 or 0x04 (encoded as 33 bytes)
 *
 * Other scripts up to 121 bytes require 1 byte + script length. Above that,
 * scripts up to 16505 bytes require 2 bytes + script length.
 */
class CScriptCompressor {
private:
    /**
     * make this static for now (there are only 6 special scripts defined) this
     * can potentially be extended together with a new nVersion for
     * transactions, in which case this value becomes dependent on nVersion and
     * nHeight of the enclosing transaction.
     */
    static const unsigned int nSpecialScripts = 6;

    CScript &script;

protected:
    /**
     * These check for scripts for which a special case with a shorter encoding
     * is defined. They are implemented separately from the CScript test, as
     * these test for exact byte sequence correspondences, and are more strict.
     * For example, IsToPubKey also verifies whether the public key is valid (as
     * invalid ones cannot be represented in compressed form).
     */
    bool IsToKeyID(CKeyID &hash) const;
    bool IsToScriptID(CScriptID &hash) const;
    bool IsToPubKey(CPubKey &pubkey) const;

    bool Compress(std::vector<uint8_t> &out) const;
    unsigned int GetSpecialSize(unsigned int nSize) const;
    bool Decompress(unsigned int nSize, const std::vector<uint8_t> &out);

public:
    CScriptCompressor(CScript &scriptIn) : script(scriptIn) {}

    template <typename Stream> void Serialize(Stream &s) const {
        std::vector<uint8_t> compr;
        if (Compress(compr)) {
            s << CFlatData(compr);
            return;
        }
        unsigned int nSize = script.size() + nSpecialScripts;
        s << VARINT(nSize);
        s << CFlatData(script);
    }

    template <typename Stream> void Unserialize(Stream &s) {
        unsigned int nSize = 0;
        s >> VARINT(nSize);
        if (nSize < nSpecialScripts) {
            std::vector<uint8_t> vch(GetSpecialSize(nSize), 0x00);
            s >> REF(CFlatData(vch));
            Decompress(nSize, vch);
            return;
        }
        nSize -= nSpecialScripts;
        if (nSize > MAX_SCRIPT_SIZE_AFTER_GENESIS) {
            // Overly long script, replace with a short invalid one
            script << OP_FALSE << OP_RETURN;
            s.ignore(nSize);
        } else {
            NonSpecialScriptUnserializer<Stream>::Unserialize(this, s, nSize);
        }
    }

private:
    /**
    * Helper class that provides a static method used to unserialize non-special script
    *
    * @note This is a separate class, so that it can be specialized for a custom Stream class, which
    *       allows customization for special cases (e.g. not loading script if it is too large).
    *       See an example in txdb.cpp.
    */
    template<typename Stream>
    class NonSpecialScriptUnserializer
    {
    public:
        static void Unserialize(CScriptCompressor* self, Stream& s, unsigned int nSize)
        {
            self->script.resize(nSize);
            s >> REF(CFlatData(self->script));
        }
    };
};

/** wrapper for CTxOut that provides a more compact serialization */
class CTxOutCompressor {
private:
    CTxOut &txout;

public:
    static uint64_t CompressAmount(Amount nAmount);
    static Amount DecompressAmount(uint64_t nAmount);

    CTxOutCompressor(CTxOut &txoutIn) : txout(txoutIn) {}

    ADD_SERIALIZE_METHODS

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action) {
        if (!ser_action.ForRead()) {
            uint64_t nVal = CompressAmount(txout.nValue);
            READWRITE(VARINT(nVal));
        } else {
            uint64_t nVal = 0;
            READWRITE(VARINT(nVal));
            txout.nValue = DecompressAmount(nVal);
        }
        CScriptCompressor cscript(REF(txout.scriptPubKey));
        READWRITE(cscript);
    }
};

#endif // MVC_COMPRESSOR_H
