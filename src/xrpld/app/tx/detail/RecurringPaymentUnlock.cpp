//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2012, 2013 Ripple Labs Inc.

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#include <xrpld/app/tx/detail/RecurringPaymentUnlock.h>
#include <xrpld/core/Config.h>
#include <xrpld/ledger/View.h>

#include <xrpl/basics/Log.h>
#include <xrpl/protocol/Feature.h>
#include <xrpl/protocol/Indexes.h>

namespace ripple {

NotTEC
RecurringPaymentUnlock::preflight(PreflightContext const& ctx)
{
    if (auto const ret = preflight1(ctx); !isTesSuccess(ret))
        return ret;

    std::uint32_t const txFlags = ctx.tx.getFlags();
    if (txFlags & tfUniversalMask)
        return temINVALID_FLAG;

    return preflight2(ctx);
}

// TxConsequences
// RecurringPaymentUnlock::makeTxConsequences(PreflightContext const& ctx)
// {
//     return TxConsequences{ctx.tx, ctx.tx[sfAmount].xrp()};
// }

TER
RecurringPaymentUnlock::checkPermission(ReadView const& view, STTx const& tx)
{
    return tesSUCCESS;
}

TER
RecurringPaymentUnlock::preclaim(PreclaimContext const& ctx)
{
    
    auto amount = ctx.tx.getFieldAmount(sfAmount);
    auto account = ctx.tx.getAccountID(sfAccount);
    auto const sle = ctx.view.read(keylet::recurringPayment(ctx.tx.getFieldH256(sfRecurringPaymentID)));
    if (!sle)
    {
        JLOG(ctx.j.error()) << "RecurringPaymentUnlock: Recurring payment not found";
        return tecNO_TARGET;
    }

    if (sle->getAccountID(sfAccount) != account)
    {
        JLOG(ctx.j.error()) << "RecurringPaymentUnlock: Account does not match the recurring payment";
        return tecNO_PERMISSION;
    }
    
    if( sle->getFieldAmount(sfLockedFunds) < amount)
    {
        JLOG(ctx.j.error()) << "RecurringPaymentUnlock: Insufficient locked funds";
        return tecINSUFFICIENT_FUNDS;
    }


    return tesSUCCESS;
}

TER
RecurringPaymentUnlock::doApply()
{
    auto amount = ctx_.tx.getFieldAmount(sfAmount);
    auto const sle = ctx_.view().peek(keylet::recurringPayment(ctx_.tx.getFieldH256(sfRecurringPaymentID)));
    auto prev_lock_funds =  sle->getFieldAmount(sfLockedFunds);
    if (prev_lock_funds < amount)
    {
        JLOG(ctx_.journal.error()) << "RecurringPaymentUnlock: Insufficient locked funds";
        return tecINSUFFICIENT_FUNDS;
    }
    // Deduct the specified amount from the locked funds
    auto new_lock_funds = prev_lock_funds - amount;
    sle->setFieldAmount(sfLockedFunds, new_lock_funds);

    // Update the acount balance
    auto account = ctx_.tx.getAccountID(sfAccount);
    auto const sleAccount = ctx_.view().peek(keylet::account(ctx_.tx.getAccountID(sfAccount)));
    sleAccount->setFieldAmount(sfBalance, sleAccount->getFieldAmount(sfBalance) + amount);

    ctx_.view().update(sle);
    return tesSUCCESS;
}

}  // namespace ripple
