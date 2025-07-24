//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2012-2016 Ripple Labs Inc.

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

#include <test/jtx.h>

#include <xrpl/protocol/TxFlags.h>
#include <xrpl/protocol/jss.h>

namespace ripple {

namespace test {

class RecurringPayment_test : public beast::unit_test::suite
{
public:

    Json::Value
    set(
        jtx::Account const& account, 
        STAmount const& amount,
        NetClock::duration const& frequency,
        jtx::Account destination)
    {
        using namespace jtx;
        Json::Value jv;
        jv[jss::TransactionType] = jss::RecurringPaymentSet;
        jv[jss::Account] = to_string(account.id());
        jv[jss::Amount] = amount.getJson(JsonOptions::none);
        jv[sfFrequency.fieldName] = frequency.count();
        jv[jss::Destination] = to_string(destination.id());
        return jv;
    }

    Json::Value
    set(
        jtx::Account const& account, 
        STAmount const& amount,
        NetClock::duration const& frequency,
        PublicKey pk)
    {
        using namespace jtx;
        Json::Value jv;
        jv[jss::TransactionType] = jss::RecurringPaymentSet;
        jv[jss::Account] = to_string(account.id());
        jv[jss::Amount] = amount.getJson(JsonOptions::none);
        jv[sfFrequency.fieldName] = frequency.count();
        jv[sfPublicKey] = strHex(pk.slice());
        return jv;
    }

    Json::Value
    lock(
        jtx::Account const& account, 
        STAmount const& amount,
        uint256 const& id)
    {
        using namespace jtx;
        Json::Value jv;
        jv[jss::TransactionType] = jss::RecurringPaymentLock;
        jv[jss::Account] = to_string(account.id());
        jv[jss::Amount] = amount.getJson(JsonOptions::none);
        jv[sfRecurringPaymentID] = to_string(id);
        return jv;
    }

    Json::Value
    unlock(
        jtx::Account const& account, 
        STAmount const& amount,
        uint256 const& id)
    {
        using namespace jtx;
        Json::Value jv;
        jv[jss::TransactionType] = jss::RecurringPaymentUnlock;
        jv[jss::Account] = to_string(account.id());
        jv[jss::Amount] = amount.getJson(JsonOptions::none);
        jv[sfRecurringPaymentID] = to_string(id);
        return jv;
    }

    Json::Value
    cancel(
        jtx::Account const& account, 
        uint256 const& id)
    {
        using namespace jtx;
        Json::Value jv;
        jv[jss::TransactionType] = jss::RecurringPaymentCancel;
        jv[jss::Account] = to_string(account.id());
        jv[sfRecurringPaymentID] = to_string(id);
        return jv;
    }

    // TODO: The destination field is redundant in the claim transaction. As the account should match the destination in the recurring payment.
    Json::Value
    claim(
        jtx::Account const& account,
        jtx::Account const& destination,
        uint256 const& id,
        STAmount const& amount)
    {
        using namespace jtx;
        Json::Value jv;
        jv[jss::TransactionType] = jss::RecurringPaymentClaim;
        jv[jss::Account] = to_string(account.id());
        jv[jss::Destination] = to_string(destination.id());
        jv[sfRecurringPaymentID] = to_string(id);
        jv[jss::Amount] = amount.getJson(JsonOptions::none);
        return jv;
    }

    Json::Value
    claim(
        jtx::Account const& account, 
        uint256 const& id,
        STAmount const& amount,
        Blob const& sig)
    {
        using namespace jtx;
        Json::Value jv;
        jv[jss::TransactionType] = jss::RecurringPaymentClaim;
        jv[jss::Account] = to_string(account.id());
        jv[sfRecurringPaymentID] = to_string(id);
        jv[jss::Amount] = amount.getJson(JsonOptions::none);
        jv[sfSignature] = strHex(sig);
        return jv;
    }

    uint256
    recurringPaymentID(
        AccountID const& account,
        AccountID const& dst,
        std::uint32_t seqProxyValue)
    {
        auto const k = keylet::recurringPayment(account, dst, seqProxyValue);
        return k.key;
    }

    void
    testEnabled(FeatureBitset features)
    {
        testcase("enabled");
        using namespace jtx;
        using namespace std::literals::chrono_literals;
        Env env(*this);
        Account const alice = Account{"alice"};
        Account const bob = Account{"bob"};
        env.fund(XRP(1000), bob, alice);
        env.close();

        //////////////////// Set
        // Using Destination
        auto const id = recurringPaymentID(alice.id(), bob.id(), env.seq(alice));
        auto const frequency = 100s;
        env(set(alice, XRP(10), frequency, bob), ter(tesSUCCESS));
        env.close();

        {
            Json::Value params;
            params[jss::ledger_index] = env.current()->seq() - 1;
            params[jss::transactions] = true;
            params[jss::expand] = true;
            auto const jrr = env.rpc("json", "ledger", to_string(params));
            std::cout << jrr << std::endl;
        }

        //////////////////// 
        
        // Claiming before locking should fail
        std::cout << "Claiming before locking should fail" << std::endl;
        env(claim(bob, bob, id, XRP(1)), ter(tecINSUFFICIENT_FUNDS));
        // Locking
        std::cout << "Locking 500 XRP" << std::endl;
        env(lock(alice, XRP(500), id), ter(tesSUCCESS));

        // Locking another 500 should fail because the total locked amount exceeds the balance
        std::cout << "Locking another 500 XRP should fail" << std::endl;
        env(lock(alice, XRP(500), id), ter(tecINSUFFICIENT_FUNDS));

        env.close();
        {
            Json::Value params;
            params[jss::ledger_index] = env.current()->seq() - 1;
            params[jss::transactions] = true;
            params[jss::expand] = true;
            auto const jrr = env.rpc("json", "ledger", to_string(params));
            std::cout << jrr << std::endl;
        }

        env(claim(alice, bob, id, XRP(1)), ter(tecNO_PERMISSION)); // Unauthorized claim should fail
        env(claim(bob, bob, id, XRP(1)), ter(tesSUCCESS));
        env(claim(bob, bob, id, XRP(10)), ter(tecINSUFFICIENT_FUNDS)); // Claiming more than the set amount should fail (it is still more than the locked funds)
        env.close();

        {
            Json::Value params;
            params[jss::ledger_index] = env.current()->seq() - 1;
            params[jss::transactions] = true;
            params[jss::expand] = true;
            auto const jrr = env.rpc("json", "ledger", to_string(params));
            std::cout << jrr << std::endl;
        }

        env(unlock(bob, XRP(1), id), ter(tecNO_PERMISSION)); //Unauthorized unlock should fail
        env(unlock(alice, XRP(500), id), ter(tecINSUFFICIENT_FUNDS)); // Unlocking more than the locked amount should fail
        env(unlock(alice, XRP(120), id), ter(tesSUCCESS)); // Unlocking 100 XRP should succeed (there should be 499 XRP left locked)
        env.close();

        {
            Json::Value params;
            params[jss::ledger_index] = env.current()->seq() - 1;
            params[jss::transactions] = true;
            params[jss::expand] = true;
            auto const jrr = env.rpc("json", "ledger", to_string(params));
            std::cout << jrr << std::endl;
        }

    }

    void
    testWithFeats(FeatureBitset features)
    {
        testEnabled(features);
    }

public:
    void
    run() override
    {
        using namespace test::jtx;
        auto const sa = testable_amendments();
        testWithFeats(sa);
    }
};
BEAST_DEFINE_TESTSUITE(RecurringPayment, app, ripple);
}  // namespace test
}  // namespace ripple
