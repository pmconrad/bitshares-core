/*
 * Copyright (c) 2017 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <graphene/app/api_objects.hpp>
#include <graphene/app/util.hpp>

namespace graphene { namespace app {

market_ticker::market_ticker(const market_ticker_object& mto,
                             const fc::time_point_sec& now,
                             const asset_object& asset_base,
                             const asset_object& asset_quote,
                             const order_book& orders)
{
   time = now;
   base = asset_base.symbol;
   quote = asset_quote.symbol;
   percent_change = "0";
   lowest_ask = "0";
   highest_bid = "0";

   fc::uint128_t bv;
   fc::uint128_t qv;
   price latest_price = asset( mto.latest_base, mto.base ) / asset( mto.latest_quote, mto.quote );
   if( mto.base != asset_base.id )
      latest_price = ~latest_price;
   latest = price_to_string( latest_price, asset_base, asset_quote );
   if( mto.last_day_base != 0 && mto.last_day_quote != 0 // has trade data before 24 hours
       && ( mto.last_day_base != mto.latest_base || mto.last_day_quote != mto.latest_quote ) ) // price changed
   {
      price last_day_price = asset( mto.last_day_base, mto.base ) / asset( mto.last_day_quote, mto.quote );
      if( mto.base != asset_base.id )
         last_day_price = ~last_day_price;
      percent_change = price_diff_percent_string( last_day_price, latest_price );
   }
   if( asset_base.id == mto.base )
   {
      bv = mto.base_volume;
      qv = mto.quote_volume;
   }
   else
   {
      bv = mto.quote_volume;
      qv = mto.base_volume;
   }
   base_volume = uint128_amount_to_string( bv, asset_base.precision );
   quote_volume = uint128_amount_to_string( qv, asset_quote.precision );

   if(!orders.asks.empty())
      lowest_ask = orders.asks[0].price;
   if(!orders.bids.empty())
      highest_bid = orders.bids[0].price;
}

market_ticker::market_ticker(const fc::time_point_sec& now,
                             const asset_object& asset_base,
                             const asset_object& asset_quote)
{
   time = now;
   base = asset_base.symbol;
   quote = asset_quote.symbol;
   latest = "0";
   lowest_ask = "0";
   highest_bid = "0";
   percent_change = "0";
   base_volume = "0";
   quote_volume = "0";
}

} } // graphene::app

FC_REFLECT_DERIVED_NO_TYPENAME( graphene::app::account_balance_api_object,
                    (graphene::chain::account_balance_master), (balance) );
FC_REFLECT_DERIVED_NO_TYPENAME( graphene::app::account_statistics_api_object,
                    (graphene::chain::account_statistics_master),
                    (pending_fees)(pending_vested_fees) );
FC_REFLECT_DERIVED_NO_TYPENAME( graphene::app::asset_bitasset_data_api_object,
                    (graphene::chain::asset_bitasset_data_master), (settlement_fund) );
FC_REFLECT_DERIVED_NO_TYPENAME( graphene::app::asset_dynamic_data_api_object,
                    (graphene::chain::asset_dynamic_data_master),
                    (current_supply)(confidential_supply)(accumulated_fees)(fee_pool) );
FC_REFLECT_DERIVED_NO_TYPENAME( graphene::app::balance_api_object,
                    (graphene::chain::balance_master), (balance) );
FC_REFLECT_DERIVED_NO_TYPENAME( graphene::app::fba_accumulator_api_object,
                    (graphene::chain::fba_accumulator_master), (accumulated_fba_fees) );
FC_REFLECT_DERIVED_NO_TYPENAME( graphene::app::limit_order_api_object,
                    (graphene::chain::limit_order_master), (for_sale)(deferred_fee)(deferred_paid_fee) );
FC_REFLECT_DERIVED_NO_TYPENAME( graphene::app::call_order_api_object,
                    (graphene::chain::call_order_master), (debt)(collateral) );
FC_REFLECT_DERIVED_NO_TYPENAME( graphene::app::force_settlement_api_object,
                    (graphene::chain::force_settlement_master), (balance) );
FC_REFLECT_DERIVED_NO_TYPENAME( graphene::app::collateral_bid_api_object,
                    (graphene::chain::collateral_bid_master), (inv_swan_price) );
FC_REFLECT_DERIVED_NO_TYPENAME( graphene::app::htlc_api_object::transfer_info,
                    (graphene::chain::htlc_master::transfer_info_master), (amount)(asset_id) );
FC_REFLECT_DERIVED_NO_TYPENAME( graphene::app::htlc_api_object,
                    (graphene::chain::htlc_master), (transfer) );
FC_REFLECT_DERIVED_NO_TYPENAME( graphene::app::dynamic_global_property_api_object,
                    (graphene::chain::dynamic_global_property_master), (witness_budget) );

GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::app::account_balance_api_object )
GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::app::account_statistics_api_object )
GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::app::asset_bitasset_data_api_object )
GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::app::asset_dynamic_data_api_object )
GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::app::balance_api_object )
GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::app::fba_accumulator_api_object )
GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::app::limit_order_api_object )
GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::app::call_order_api_object )
GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::app::force_settlement_api_object )
GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::app::collateral_bid_api_object )
GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::app::htlc_api_object::transfer_info )
GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::app::htlc_api_object )
GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::app::vesting_balance_api_object )
GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::app::dynamic_global_property_api_object )
