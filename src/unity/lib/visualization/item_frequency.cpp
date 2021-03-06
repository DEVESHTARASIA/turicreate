/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include "item_frequency.hpp"
#include "vega_spec.hpp"

using namespace turi::visualization;

std::vector<item_frequency_result> item_frequency::split_input(size_t num_threads) {
  // TODO - do we need to do anything here? perhaps not.
  return std::vector<item_frequency_result>(num_threads);
}

void item_frequency::merge_results(std::vector<item_frequency_result>& transformers) {
  for (const auto& other : transformers) {
    this->m_transformer->combine(other);
  }
}

void item_frequency_result::add_element_simple(const flexible_type& flex) {
  groupby_operators::frequency_count::add_element_simple(flex);

  /*
   * add element to summary stats
   */
  m_count.add_element_simple(flex);
  m_count_distinct.add_element_simple(flex);
  m_non_null_count.add_element_simple(flex);
}

void item_frequency_result::combine(const group_aggregate_value& other) {
  groupby_operators::frequency_count::combine(other);

  /* combine summary stats */
  auto item_frequency_other = dynamic_cast<const item_frequency_result&>(other);
  m_count.combine(item_frequency_other.m_count);
  m_count_distinct.combine(item_frequency_other.m_count_distinct);
  m_non_null_count.combine(item_frequency_other.m_non_null_count);
}

std::string item_frequency_result::vega_summary_data() const {
  std::stringstream ss;

  flex_int num_missing = m_count.emit() - m_non_null_count.emit();
  std::string data = vega_column_data(true);

  ss << "\"type\": \"str\",";
  ss << "\"num_unique\": " << m_count_distinct.emit() << ",";
  ss << "\"num_missing\": " << num_missing << ",";
  ss << "\"categorical\": [" << data << "],";
  ss << "\"numeric\": []";

  return ss.str();

}

std::string item_frequency_result::vega_column_data(bool sframe) const {
  std::stringstream ss;
  size_t x = 0;

  auto items_list = emit().get<flex_dict>();
  size_t size_list;

  if(sframe){
    size_list = std::min(10UL, items_list.size());
  }else{
    size_list = std::min(200UL, items_list.size());
  }

  std::sort(items_list.begin(), items_list.end(), [](const std::pair<turi::flexible_type,flexible_type> &left, const std::pair<turi::flexible_type,flexible_type> &right) {
    if (left.second == right.second) {
      // if count is equal, sort ascending by label
      return right.first > left.first;
    }
    // sort descending by count
    return left.second > right.second;
  });

  for(size_t i=0; i<size_list; i++) {

    const auto& pair = items_list[i];
    const auto& flex_value = pair.first;
    if (flex_value.get_type() == flex_type_enum::UNDEFINED) {
      // skip missing values for now
      continue;
    }

    DASSERT_TRUE(flex_value.get_type() == flex_type_enum::STRING);
    const auto& value = flex_value.get<flex_string>();

    size_t count = pair.second.get<flex_int>();

    ss << "{\"label\": ";
    ss << escape_string(value);
    ss << ",\"label_idx\": ";
    ss << i;
    ss << ",\"count\": ";
    ss << count;
    ss << "}";

    if(x != (size_list - 1)){
      ss << ",";
    }

    x++;
  }

  return ss.str();
}
