#include <xrpld/app/tx/detail/SetQuantumKey.h>
#include <xrpl/protocol/Feature.h>
#include <xrpl/protocol/KeyType.h>
#include <xrpl/protocol/PublicKey.h>
#include <xrpl/protocol/Indexes.h>

extern "C" {
#include "api.h"
}

#ifndef DILITHIUM_PK_SIZE
#define DILITHIUM_PK_SIZE pqcrystals_dilithium2_PUBLICKEYBYTES 
#endif

namespace ripple {

NotTEC
SetQuantumKey::preflight(PreflightContext const& ctx)
{
    if (!ctx.rules.enabled(featureQuantum))
        return temDISABLED;

    if (auto const ret = preflight1(ctx); !isTesSuccess(ret))
        return ret;

    // TODO: Validate quantum public key format and size
    if (! ctx.tx.isFieldPresent (sfQuantumPublicKey))
    {        
        JLOG(ctx.j.warn()) << "Missing sfQuantumPublicKey field";
        return temMALFORMED;
    }

    auto const& pkQuantum = ctx.tx.getFieldVL(sfQuantumPublicKey);
    if (pkQuantum.size() != DILITHIUM_PK_SIZE)
    {
        JLOG(ctx.j.warn()) << "Invalid quantum public key size: "
                           << pkQuantum.size();
        return temMALFORMED;
    }

    
    // Hint: Check sfQuantumPublicKey field exists and is valid Dilithium key

    return preflight2(ctx);
}

TER
SetQuantumKey::preclaim(PreclaimContext const& ctx)
{
    // TODO: Check if quantum key already exists for this account
    // Hint: Use keylet::quantum() to check if ledger entry exists

    Slice const pkSlice {
        ctx.tx.getFieldVL (sfQuantumPublicKey).data(),
        ctx.tx.getFieldVL (sfQuantumPublicKey).size()
    };
    auto const idAccount = ctx.tx.getAccountID(sfAccount);
    auto const sleAccount = ctx.view.read(keylet::quantum(idAccount, pkSlice));
    if (!sleAccount)
    {
        JLOG(ctx.j.warn())
            << "applyTransaction: source account does not exist "
            << toBase58(idAccount);
        return terNO_ACCOUNT;
    }
    
    return tesSUCCESS;
}

TER
SetQuantumKey::doApply()
{
    // TODO: Create or update the quantum key ledger entry
    // Hint: Use keylet::quantum() to create the entry
    // Set all required fields: sfAccount, sfQuantumPublicKey, etc.
    auto const  idAccount = ctx_.tx.getAccountID (sfAccount);
    Blob const& pkBlob  = ctx_.tx.getFieldVL (sfQuantumPublicKey);
    Slice const pkSlice { pkBlob.data(), pkBlob.size() };
    auto const kq = keylet::quantum (idAccount, pkSlice);

    auto sle = std::make_shared<SLE> (kq);

    if ( ctx_.tx.isFieldPresent (sfQuantumPublicKey))
    {
        sle->setAccountID(sfAccount, idAccount);
        sle->setFieldVL   (sfQuantumPublicKey,  pkBlob);
    }
    
    return tesSUCCESS;
}

} // namespace ripple