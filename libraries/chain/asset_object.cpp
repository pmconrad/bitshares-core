/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
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
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/hardfork.hpp>

#include <fc/io/raw.hpp>
#include <fc/uint128.hpp>

namespace graphene { namespace chain {

class asset_dynamic_data_backup
   : public asset_dynamic_data_master, public backup_object<asset_dynamic_data_object>
{
      asset current_supply;
      share_type accumulated_fees;
      share_type fee_pool;
      share_type confidential_supply;
      friend class asset_dynamic_data_object;

   public:
      asset_dynamic_data_backup( const asset_dynamic_data_object& original )
         : asset_dynamic_data_master( original )
      {
         current_supply = original.current_supply.get_value();
         accumulated_fees = original.accumulated_fees.get_amount();
         fee_pool = original.fee_pool.get_amount();
         confidential_supply = original.confidential_supply.get_amount();
      }

      virtual object* recreate() { return graphene::db::backup_object<asset_dynamic_data_object>::recreate(); }
};

unique_ptr<object> asset_dynamic_data_object::backup()const
{
   return std::make_unique<asset_dynamic_data_backup>( *this );
}

void asset_dynamic_data_object::restore( object& obj )
{
   const auto& backup = static_cast<asset_dynamic_data_backup&>(obj);
   current_supply.restore( backup.current_supply );
   accumulated_fees.restore( asset( backup.accumulated_fees, backup.current_supply.asset_id ) );
   fee_pool.restore( asset( backup.fee_pool ) );
   confidential_supply.restore( asset( backup.confidential_supply, backup.current_supply.asset_id ) );
   static_cast<asset_dynamic_data_master&>(*this) = std::move( backup );
}

void asset_dynamic_data_object::clear()
{
   current_supply.clear();
   accumulated_fees.clear();
   fee_pool.clear();
   confidential_supply.clear();
}

share_type asset_bitasset_data_master::max_force_settlement_volume(share_type current_supply) const
{
   if( options.maximum_force_settlement_volume == 0 )
      return 0;
   if( options.maximum_force_settlement_volume == GRAPHENE_100_PERCENT )
      return current_supply + force_settled_volume;

   fc::uint128_t volume = current_supply.value;
   volume += force_settled_volume.value;
   volume *= options.maximum_force_settlement_volume;
   volume /= GRAPHENE_100_PERCENT;
   return static_cast<uint64_t>(volume);
}

void graphene::chain::asset_bitasset_data_master::update_median_feeds( time_point_sec current_time,
                                                                       time_point_sec next_maintenance_time )
{
   bool after_core_hardfork_1270 = ( next_maintenance_time > HARDFORK_CORE_1270_TIME ); // call price caching issue
   current_feed_publication_time = current_time;
   vector<std::reference_wrapper<const price_feed>> current_feeds;
   // find feeds that were alive at current_time
   for( const pair<account_id_type, pair<time_point_sec,price_feed>>& f : feeds )
   {
      if( (current_time - f.second.first).to_seconds() < options.feed_lifetime_sec &&
          f.second.first != time_point_sec() )
      {
         current_feeds.emplace_back(f.second.second);
         current_feed_publication_time = std::min(current_feed_publication_time, f.second.first);
      }
   }

   // If there are no valid feeds, or the number available is less than the minimum to calculate a median...
   if( current_feeds.size() < options.minimum_feeds )
   {
      //... don't calculate a median, and set a null feed
      feed_cer_updated = false; // new median cer is null, won't update asset_object anyway, set to false for better performance
      current_feed_publication_time = current_time;
      current_feed = price_feed();
      if( after_core_hardfork_1270 )
         current_maintenance_collateralization = price();
      return;
   }
   if( current_feeds.size() == 1 )
   {
      if( current_feed.core_exchange_rate != current_feeds.front().get().core_exchange_rate )
         feed_cer_updated = true;
      current_feed = current_feeds.front();
      // Note: perhaps can defer updating current_maintenance_collateralization for better performance
      if( after_core_hardfork_1270 )
         current_maintenance_collateralization = current_feed.maintenance_collateralization();
      return;
   }

   // *** Begin Median Calculations ***
   price_feed median_feed;
   const auto median_itr = current_feeds.begin() + current_feeds.size() / 2;
#define CALCULATE_MEDIAN_VALUE(r, data, field_name) \
   std::nth_element( current_feeds.begin(), median_itr, current_feeds.end(), \
                     [](const price_feed& a, const price_feed& b) { \
      return a.field_name < b.field_name; \
   }); \
   median_feed.field_name = median_itr->get().field_name;

   BOOST_PP_SEQ_FOR_EACH( CALCULATE_MEDIAN_VALUE, ~, GRAPHENE_PRICE_FEED_FIELDS )
#undef CALCULATE_MEDIAN_VALUE
   // *** End Median Calculations ***

   if( current_feed.core_exchange_rate != median_feed.core_exchange_rate )
      feed_cer_updated = true;
   current_feed = median_feed;
   // Note: perhaps can defer updating current_maintenance_collateralization for better performance
   if( after_core_hardfork_1270 )
      current_maintenance_collateralization = current_feed.maintenance_collateralization();
}

class asset_bitasset_data_backup
   : public asset_bitasset_data_master, public graphene::db::backup_object<asset_bitasset_data_object>
{
      share_type settlement_fund;
      share_type total_debt;
      friend class asset_bitasset_data_object;

   public:
      asset_bitasset_data_backup( const asset_bitasset_data_object& original )
         : asset_bitasset_data_master( original )
      {
         settlement_fund = original.settlement_fund.get_amount();
         total_debt = original.total_debt.get_amount();
      }

      virtual object* recreate() { return graphene::db::backup_object<asset_bitasset_data_object>::recreate(); }
};

unique_ptr<object> asset_bitasset_data_object::backup()const
{
   return std::make_unique<asset_bitasset_data_backup>( *this );
}

void asset_bitasset_data_object::restore( object& obj )
{
   const auto& backup = static_cast<asset_bitasset_data_backup&>(obj);
   settlement_fund.restore( asset( backup.settlement_fund, backup.options.short_backing_asset ) );
   total_debt.restore( asset( backup.total_debt, backup.asset_id ) );
   static_cast<asset_bitasset_data_master&>(*this) = std::move( backup );
}

void asset_bitasset_data_object::clear()
{
   settlement_fund.clear();
   total_debt.clear();
}


asset asset_object::amount_from_string(string amount_string) const
{ try {
   bool negative_found = false;
   bool decimal_found = false;
   for( const char c : amount_string )
   {
      if( isdigit( c ) )
         continue;

      if( c == '-' && !negative_found )
      {
         negative_found = true;
         continue;
      }

      if( c == '.' && !decimal_found )
      {
         decimal_found = true;
         continue;
      }

      FC_THROW( (amount_string) );
   }

   share_type satoshis = 0;

   share_type scaled_precision = asset::scaled_precision( precision );

   const auto decimal_pos = amount_string.find( '.' );
   const string lhs = amount_string.substr( negative_found, decimal_pos );
   if( !lhs.empty() )
      satoshis += fc::safe<int64_t>(std::stoll(lhs)) *= scaled_precision;

   if( decimal_found )
   {
      const size_t max_rhs_size = std::to_string( scaled_precision.value ).substr( 1 ).size();

      string rhs = amount_string.substr( decimal_pos + 1 );
      FC_ASSERT( rhs.size() <= max_rhs_size );

      while( rhs.size() < max_rhs_size )
         rhs += '0';

      if( !rhs.empty() )
         satoshis += std::stoll( rhs );
   }

   FC_ASSERT( satoshis <= GRAPHENE_MAX_SHARE_SUPPLY );

   if( negative_found )
      satoshis *= -1;

   return amount(satoshis);
} FC_CAPTURE_AND_RETHROW( (amount_string) ) }

string asset_object::amount_to_string(share_type amount) const
{
   share_type scaled_precision = asset::scaled_precision( precision );

   string result = fc::to_string(amount.value / scaled_precision.value);
   auto decimals = abs( amount.value % scaled_precision.value );
   if( decimals )
   {
      if( amount < 0 && result == "0" )
         result = "-0";
      result += "." + fc::to_string(scaled_precision.value + decimals).erase(0,1);
   }
   return result;
}

} } // graphene::chain

FC_REFLECT_DERIVED_NO_TYPENAME( graphene::chain::asset_dynamic_data_object,
                    (graphene::chain::asset_dynamic_data_master),
                    (current_supply)(accumulated_fees)(fee_pool)(confidential_supply)(borrowed_fees) )

FC_REFLECT_DERIVED_NO_TYPENAME( graphene::chain::asset_bitasset_data_object,
                    (graphene::chain::asset_bitasset_data_master),
                    (settlement_fund)
                    (total_debt)
                  )

GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::chain::asset_object )
GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::chain::asset_bitasset_data_master )
GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::chain::asset_bitasset_data_object )
GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::chain::asset_dynamic_data_master )
GRAPHENE_IMPLEMENT_EXTERNAL_SERIALIZATION( graphene::chain::asset_dynamic_data_object )
