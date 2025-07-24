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

#include <xrpld/app/tx/detail/RecurringPaymentCancel.h>
#include <xrpld/core/Config.h>
#include <xrpld/ledger/View.h>

#include <xrpl/basics/Log.h>
#include <xrpl/protocol/Feature.h>
#include <xrpl/protocol/Indexes.h>

namespace ripple {

NotTEC
RecurringPaymentCancel::preflight(PreflightContext const& ctx)
{
    if (auto const ret = preflight1(ctx); !isTesSuccess(ret))
        return ret;

    std::uint32_t const txFlags = ctx.tx.getFlags();
    if (txFlags & tfUniversalMask)
        return temINVALID_FLAG;

    return preflight2(ctx);
}

// TxConsequences
// RecurringPaymentCancel::makeTxConsequences(PreflightContext const& ctx)
// {
//     return TxConsequences{ctx.tx, ctx.tx[sfAmount].xrp()};
// }

TER
RecurringPaymentCancel::checkPermission(ReadView const& view, STTx const& tx)
{
    return tesSUCCESS;
}

TER
RecurringPaymentCancel::preclaim(PreclaimContext const& ctx)
{
    return tesSUCCESS;
}

TER
RecurringPaymentCancel::doApply()
{
    if (!ctx_.tx.isFieldPresent(sfRecurringPaymentID))
    {
        JLOG(ctx_.journal.error()) << "RecurringPaymentCancel: RecurringPaymentID is not present";
        return temMALFORMED;
    }

    auto const sle = ctx_.view().peek(keylet::recurringPayment(ctx_.tx.getFieldH256(sfRecurringPaymentID)));
    if (!sle)
    {
        JLOG(ctx_.journal.error()) << "RecurringPaymentCancel: Recurring payment not found";
        return tecNO_TARGET;
    }

    auto const sleAccount = ctx_.view().peek(keylet::account(ctx_.tx.getAccountID(sfAccount)));
    sleAccount->setFieldAmount(sfBalance, sleAccount->getFieldAmount(sfBalance) + sle->getFieldAmount(sfLockedFunds));

    sle->setFieldAmount(sfLockedFunds, XRPAmount(0));

    ctx_.view().erase(sle);

    return tesSUCCESS;
}

}  // namespace ripple
