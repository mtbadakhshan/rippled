#include <test/jtx.h>
#include <test/jtx/Env.h>
#include <test/jtx/Account.h>
#include <test/jtx/pay.h>
#include <test/jtx/flags.h>

#include <xrpld/app/tx/apply.h>

#include <xrpl/protocol/Feature.h>
#include <xrpl/protocol/jss.h>

#include <xrpld/core/ConfigSections.h>




namespace ripple {
namespace test {

class MyTests_test: public beast::unit_test::suite
{
    public:
    void
    testSignatures(FeatureBitset features)
    {
        testcase("Signatures");

        using namespace jtx;

        Env env{*this, features};
        Account const alice{"alice", KeyType::dilithium};
        Account const bob{"bob"};
        Account const carol{"carol"};
        Account const eve{"eve"};
        env.fund(XRP(5000), alice, bob, carol, eve);
        env.close();

        std::cout << "START" << std::endl;

        {
            env(fset(alice, asfForceQuantum));
            env(pay(alice, bob, XRP(100)));
            env.close();
        }


        Json::Value params;
        params[jss::ledger_index] = env.current()->seq() - 1;
        params[jss::transactions] = true;
        params[jss::expand] = true;
        auto const jrr = env.rpc("json", "ledger", to_string(params));
        std::cout << jrr << std::endl;
    }


    void
    testQuantum(FeatureBitset features)
    {
        using namespace test::jtx;

        testcase("quantum");

        Env env{*this, envconfig(), features};
        Account const alice{"alice"};
        Account const bob{"bob"};
        Account const carol{"carol"};
        Account const dave{"dave", KeyType::dilithium};
        env.fund(XRP(1000), alice, bob, carol, dave);
        env.close();

        Json::Value jv;
        jv[sfAccount.jsonName] = alice.human();
        jv[sfQuantumPublicKey.jsonName] = strHex(dave.pk().slice());
        jv[sfTransactionType.jsonName] = jss::SetQuantumKey;

        env(jv);
        env.close();

        // Json::Value params;
        // params[jss::ledger_index] = env.current()->seq() - 1;
        // params[jss::transactions] = true;
        // params[jss::expand] = true;
        // auto const jrr = env.rpc("json", "ledger", to_string(params));
        // std::cout << jrr << std::endl;
    } 


    void
    run() override
    {
        using namespace jtx;

        // testSignatures(FeatureBitset{});
        testQuantum(FeatureBitset{});
    }
};
BEAST_DEFINE_TESTSUITE(MyTests, bootcamp, ripple);
}
}