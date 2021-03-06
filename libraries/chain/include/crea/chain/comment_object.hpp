#pragma once

#include <crea/protocol/authority.hpp>
#include <crea/protocol/crea_operations.hpp>

#include <crea/chain/crea_object_types.hpp>
#include <crea/chain/witness_objects.hpp>
#include <fc/string.hpp>

#include <boost/multi_index/composite_key.hpp>


namespace crea { namespace chain {

   using protocol::beneficiary_route_type;
   using chainbase::t_vector;
   using chainbase::t_pair;
#ifdef CREA_ENABLE_SMT
   using protocol::votable_asset_info;
#endif

   struct strcmp_less
   {
      bool operator()( const shared_string& a, const shared_string& b )const
      {
         return less( a.c_str(), b.c_str() );
      }

#ifndef ENABLE_STD_ALLOCATOR
      bool operator()( const shared_string& a, const string& b )const
      {
         return less( a.c_str(), b.c_str() );
      }

      bool operator()( const string& a, const shared_string& b )const
      {
         return less( a.c_str(), b.c_str() );
      }
#endif

      private:
         inline bool less( const char* a, const char* b )const
         {
            return std::strcmp( a, b ) < 0;
         }
   };

   class comment_object : public object < comment_object_type, comment_object >
   {
      comment_object() = delete;

      public:
         template< typename Constructor, typename Allocator >
         comment_object( Constructor&& c, allocator< Allocator > a )
            :category( a ), parent_permlink( a ), permlink( a ), beneficiaries( a )
#ifdef CREA_ENABLE_SMT
            , allowed_vote_assets( a )
#endif
         {
            c( *this );
         }

         id_type           id;

         shared_string     category;
         account_name_type parent_author;
         shared_string     parent_permlink;
         account_name_type author;
         shared_string     permlink;

         time_point_sec    last_update;
         time_point_sec    created;
         time_point_sec    active; ///< the last time this post was "touched" by voting or reply
         time_point_sec    last_payout;

         uint16_t          depth = 0; ///< used to track max nested depth
         uint32_t          children = 0; ///< used to track the total number of children, grandchildren, etc...

         /// index on pending_payout for "things happning now... needs moderation"
         /// TRENDING = UNCLAIMED + PENDING
         share_type        net_rshares; // reward is proportional to rshares^2, this is the sum of all votes (positive and negative)
         share_type        abs_rshares; /// this is used to track the total abs(weight) of votes for the purpose of calculating cashout_time
         share_type        vote_rshares; /// Total positive rshares from all votes. Used to calculate delta weights. Needed to handle vote changing and removal.

         share_type        children_abs_rshares; /// this is used to calculate cashout time of a discussion.
         time_point_sec    cashout_time; /// 24 hours from the weighted average of vote time
         time_point_sec    max_cashout_time;
         uint64_t          total_vote_weight = 0; /// the total weight of voting rewards, used to calculate pro-rata share of curation payouts

         uint16_t          reward_weight = 0;

         /** tracks the total payout this comment has received over time, measured in CBD */
         asset             total_payout_value = asset(0, CBD_SYMBOL);
         asset             curator_payout_value = asset(0, CBD_SYMBOL);
         asset             beneficiary_payout_value = asset( 0, CBD_SYMBOL );

         share_type        author_rewards = 0;

         int32_t           net_votes = 0;

         id_type           root_comment;

         asset             max_accepted_payout = asset( 1000000000, CBD_SYMBOL );       /// CBD value of the maximum payout this post will receive
         uint16_t          percent_crea_dollars = CREA_100_PERCENT; /// the percent of Crea Dollars to key, unkept amounts will be received as Crea Power
         bool              allow_replies = true;      /// allows a post to disable replies.
         bool              allow_votes   = true;      /// allows a post to receive votes;
         bool              allow_curation_rewards = true;

         using t_beneficiaries = t_vector< beneficiary_route_type >;
         t_beneficiaries   beneficiaries;
#ifdef CREA_ENABLE_SMT
         using t_votable_assets = t_vector< t_pair< asset_symbol_type, votable_asset_info > >;
         t_votable_assets  allowed_vote_assets;
#endif
   };

   class comment_content_object : public object< comment_content_object_type, comment_content_object >
   {
      comment_content_object() = delete;

      public:
         template< typename Constructor, typename Allocator >
         comment_content_object( Constructor&& c, allocator< Allocator > a ) :
            title( a ), body( a ), json_metadata( a )
         {
            c( *this );
         }

         id_type           id;

         comment_id_type   comment;

         shared_string     title;
         shared_string     body;
         shared_string     json_metadata;
   };

   class comment_download_object : public object< comment_download_object_type, comment_download_object >
   {
       comment_download_object() = delete;

       public:
          template< typename Constructor, typename Allocator >
          comment_download_object (Constructor&& c, allocator< Allocator > a ) :
             resource( a ), name( a ), type( a ), password( a )
          {
              c( *this );
          }

          id_type           id;
          comment_id_type   comment;

          shared_string     resource;
          shared_string     name;
          shared_string     type;
          uint32_t          size = 0;
          uint32_t          times_downloaded = 0;
          shared_string     password;
          asset             price;

   };

   class download_granted_object : public object< download_granted_object_type, download_granted_object >
   {
       download_granted_object() = delete;

       public:
          template< typename Constructor, typename Allocator >
          download_granted_object (Constructor&& c, allocator< Allocator > a) :
             comment_permlink( a )
          {
              c ( *this );
          }

          id_type                   id;
          comment_download_id_type  download;

          time_point_sec            payment_date;
          account_name_type         downloader;
          account_name_type         comment_author;
          shared_string             comment_permlink;
          asset                     price;
   };

   /**
    * Used to parse download data
    */
   struct comment_download_data {

       string resource;
       string name;
       string type;
       uint32_t size;
       string price;
   };

   /**
    * This index maintains the set of voter/comment pairs that have been used, voters cannot
    * vote on the same comment more than once per payout period.
    */
   class comment_vote_object : public object< comment_vote_object_type, comment_vote_object>
   {
      public:
         template< typename Constructor, typename Allocator >
         comment_vote_object( Constructor&& c, allocator< Allocator > a )
         {
            c( *this );
         }

         id_type           id;

         account_id_type   voter;
         comment_id_type   comment;
         uint64_t          weight = 0; ///< defines the score this vote receives, used by vote payout calc. 0 if a negative vote or changed votes.
         int64_t           rshares = 0; ///< The number of rshares this vote is responsible for
         int16_t           vote_percent = 0; ///< The percent weight of the vote
         time_point_sec    last_update; ///< The time of the last update of the vote
         int8_t            num_changes = 0;
   };

   struct by_comment_voter;
   struct by_voter_comment;
   typedef multi_index_container<
      comment_vote_object,
      indexed_by<
         ordered_unique< tag< by_id >, member< comment_vote_object, comment_vote_id_type, &comment_vote_object::id > >,
         ordered_unique< tag< by_comment_voter >,
            composite_key< comment_vote_object,
               member< comment_vote_object, comment_id_type, &comment_vote_object::comment>,
               member< comment_vote_object, account_id_type, &comment_vote_object::voter>
            >
         >,
         ordered_unique< tag< by_voter_comment >,
            composite_key< comment_vote_object,
               member< comment_vote_object, account_id_type, &comment_vote_object::voter>,
               member< comment_vote_object, comment_id_type, &comment_vote_object::comment>
            >
         >
      >,
      allocator< comment_vote_object >
   > comment_vote_index;


   struct by_cashout_time; /// cashout_time
   struct by_permlink; /// author, perm
   struct by_root;
   struct by_parent;
   struct by_last_update; /// parent_auth, last_update
   struct by_author_last_update;

   /**
    * @ingroup object_index
    */
   typedef multi_index_container<
      comment_object,
      indexed_by<
         /// CONSENSUS INDICES - used by evaluators
         ordered_unique< tag< by_id >, member< comment_object, comment_id_type, &comment_object::id > >,
         ordered_unique< tag< by_cashout_time >,
            composite_key< comment_object,
               member< comment_object, time_point_sec, &comment_object::cashout_time>,
               member< comment_object, comment_id_type, &comment_object::id >
            >
         >,
         ordered_unique< tag< by_permlink >, /// used by consensus to find posts referenced in ops
            composite_key< comment_object,
               member< comment_object, account_name_type, &comment_object::author >,
               member< comment_object, shared_string, &comment_object::permlink >
            >,
            composite_key_compare< std::less< account_name_type >, strcmp_less >
         >,
         ordered_unique< tag< by_root >,
            composite_key< comment_object,
               member< comment_object, comment_id_type, &comment_object::root_comment >,
               member< comment_object, comment_id_type, &comment_object::id >
            >
         >,
         ordered_unique< tag< by_parent >, /// used by consensus to find posts referenced in ops
            composite_key< comment_object,
               member< comment_object, account_name_type, &comment_object::parent_author >,
               member< comment_object, shared_string, &comment_object::parent_permlink >,
               member< comment_object, comment_id_type, &comment_object::id >
            >,
            composite_key_compare< std::less< account_name_type >, strcmp_less, std::less< comment_id_type > >
         >
         /// NON_CONSENSUS INDICIES - used by APIs
#ifndef IS_LOW_MEM
         ,
         ordered_unique< tag< by_last_update >,
            composite_key< comment_object,
               member< comment_object, account_name_type, &comment_object::parent_author >,
               member< comment_object, time_point_sec, &comment_object::last_update >,
               member< comment_object, comment_id_type, &comment_object::id >
            >,
            composite_key_compare< std::less< account_name_type >, std::greater< time_point_sec >, std::less< comment_id_type > >
         >,
         ordered_unique< tag< by_author_last_update >,
            composite_key< comment_object,
               member< comment_object, account_name_type, &comment_object::author >,
               member< comment_object, time_point_sec, &comment_object::last_update >,
               member< comment_object, comment_id_type, &comment_object::id >
            >,
            composite_key_compare< std::less< account_name_type >, std::greater< time_point_sec >, std::less< comment_id_type > >
         >
#endif
      >,
      allocator< comment_object >
   > comment_index;

   struct by_comment;
   struct by_download;
   struct by_downloader;

   typedef multi_index_container<
      comment_content_object,
      indexed_by<
         ordered_unique< tag< by_id >, member< comment_content_object, comment_content_id_type, &comment_content_object::id > >,
         ordered_unique< tag< by_comment >, member< comment_content_object, comment_id_type, &comment_content_object::comment > >
      >,
      allocator< comment_content_object >
   > comment_content_index;

   typedef multi_index_container<
      comment_download_object,
      indexed_by<
         ordered_unique< tag< by_id >, member< comment_download_object, comment_download_id_type, &comment_download_object::id > >,
         ordered_unique< tag< by_comment >, member< comment_download_object, comment_id_type, &comment_download_object::comment > >
      >,
      allocator< comment_download_object >
   > comment_download_index;

   typedef multi_index_container<
      download_granted_object,
        indexed_by<
           ordered_unique< tag< by_id >, member< download_granted_object, download_granted_id_type, &download_granted_object::id > >,
           ordered_unique< tag< by_download >,
              composite_key< download_granted_object,
                 member< download_granted_object, download_granted_id_type, &download_granted_object::id >,
                 member< download_granted_object, comment_download_id_type, &download_granted_object::download >
              >
           >,
           ordered_unique< tag< by_downloader >,
              composite_key< download_granted_object,
                 member< download_granted_object, account_name_type, &download_granted_object::comment_author >,
                 member< download_granted_object, shared_string, &download_granted_object::comment_permlink >,
                 member< download_granted_object, account_name_type, &download_granted_object::downloader >
              >,
              composite_key_compare< std::less< account_name_type >, strcmp_less, std::less< account_name_type > >
           >
        >,
      allocator< download_granted_object >
   > download_granted_index;

} } // crea::chain

FC_REFLECT( crea::chain::comment_object,
             (id)(author)(permlink)
             (category)(parent_author)(parent_permlink)
             (last_update)(created)(active)(last_payout)
             (depth)(children)
             (net_rshares)(abs_rshares)(vote_rshares)
             (children_abs_rshares)(cashout_time)(max_cashout_time)
             (total_vote_weight)(reward_weight)(total_payout_value)(curator_payout_value)(beneficiary_payout_value)(author_rewards)(net_votes)(root_comment)
             (max_accepted_payout)(percent_crea_dollars)(allow_replies)(allow_votes)(allow_curation_rewards)
             (beneficiaries)
#ifdef CREA_ENABLE_SMT
             (allowed_vote_assets)
#endif
          )

CHAINBASE_SET_INDEX_TYPE( crea::chain::comment_object, crea::chain::comment_index )

FC_REFLECT( crea::chain::comment_content_object,
            (id)(comment)(title)(body)(json_metadata) )
CHAINBASE_SET_INDEX_TYPE( crea::chain::comment_content_object, crea::chain::comment_content_index )

FC_REFLECT( crea::chain::download_granted_object,
            (id)(payment_date)(downloader)(comment_author)(comment_permlink)(price) )

CHAINBASE_SET_INDEX_TYPE( crea::chain::download_granted_object, crea::chain::download_granted_index )

FC_REFLECT( crea::chain::comment_download_data,
            (resource)(name)(type)(size)(price) )

FC_REFLECT( crea::chain::comment_download_object,
            (id)(comment)(resource)(name)(type)(size)(times_downloaded)(password)(price) )

CHAINBASE_SET_INDEX_TYPE( crea::chain::comment_download_object, crea::chain::comment_download_index )

FC_REFLECT( crea::chain::comment_vote_object,
             (id)(voter)(comment)(weight)(rshares)(vote_percent)(last_update)(num_changes)
          )
CHAINBASE_SET_INDEX_TYPE( crea::chain::comment_vote_object, crea::chain::comment_vote_index )

namespace helpers
{
   using crea::chain::shared_string;
   template <>
   class index_statistic_provider<crea::chain::comment_index>
   {
   public:
      typedef crea::chain::comment_index IndexType;
      typedef typename crea::chain::comment_object::t_beneficiaries t_beneficiaries;
#ifdef CREA_ENABLE_SMT
      typedef typename crea::chain::comment_object::t_votable_assets t_votable_assets;
#endif
      index_statistic_info gather_statistics(const IndexType& index, bool onlyStaticInfo) const
      {
         index_statistic_info info;
         gather_index_static_data(index, &info);

         if(onlyStaticInfo == false)
         {
            for(const auto& o : index)
            {
               info._item_additional_allocation += o.category.capacity()*sizeof(shared_string::value_type);
               info._item_additional_allocation += o.parent_permlink.capacity()*sizeof(shared_string::value_type);
               info._item_additional_allocation += o.permlink.capacity()*sizeof(shared_string::value_type);
               info._item_additional_allocation += o.beneficiaries.capacity()*sizeof(t_beneficiaries::value_type);
#ifdef CREA_ENABLE_SMT
               info._item_additional_allocation += o.allowed_vote_assets.capacity()*sizeof(t_votable_assets::value_type);
#endif
            }
         }

         return info;
      }
   };

   template <>
   class index_statistic_provider<crea::chain::comment_content_index>
   {
   public:
      typedef crea::chain::comment_content_index IndexType;

      index_statistic_info gather_statistics(const IndexType& index, bool onlyStaticInfo) const
      {
         index_statistic_info info;
         gather_index_static_data(index, &info);

         if(onlyStaticInfo == false)
         {
            for(const auto& o : index)
            {
               info._item_additional_allocation += o.title.capacity()*sizeof(shared_string::value_type);
               info._item_additional_allocation += o.body.capacity()*sizeof(shared_string::value_type);
               info._item_additional_allocation += o.json_metadata.capacity()*sizeof(shared_string::value_type);
            }
         }

         return info;
      }
   };

    template <>
    class index_statistic_provider<crea::chain::comment_download_index>
    {
    public:
        typedef crea::chain::comment_download_index IndexType;

        index_statistic_info gather_statistics(const IndexType& index, bool onlyStaticInfo) const
        {
            index_statistic_info info;
            gather_index_static_data(index, &info);

            if(onlyStaticInfo == false)
            {
                for(const auto& o : index)
                {
                    info._item_additional_allocation += o.resource.capacity()*sizeof(shared_string::value_type);
                    info._item_additional_allocation += o.type.capacity()*sizeof(shared_string::value_type);
                    info._item_additional_allocation += o.name.capacity()*sizeof(shared_string::value_type);
                    info._item_additional_allocation += o.password.capacity()*sizeof(shared_string::value_type);
                    info._item_additional_allocation += sizeof(o.size);
                    info._item_additional_allocation += sizeof(o.times_downloaded);
                }
            }

            return info;
        }
    };

    template <>
    class index_statistic_provider<crea::chain::download_granted_index>
    {
    public:
        typedef crea::chain::download_granted_index IndexType;

        index_statistic_info gather_statistics(const IndexType& index, bool onlyStaticInfo) const
        {
            index_statistic_info info;
            gather_index_static_data(index, &info);

            if(onlyStaticInfo == false)
            {
                for(const auto& o : index)
                {
                    info._item_additional_allocation += o.comment_permlink.capacity()*sizeof(shared_string::value_type);
                }
            }

            return info;
        }
    };

} /// namespace helpers
