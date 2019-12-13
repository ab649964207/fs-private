
#pragma once

#include <memory>
#include <vector>

#include <fs/core/utils/sdd.hxx>


namespace fs0 {
	class State;
	class LiftedActionID;
}

namespace fs0::language::fstrips { class Formula; }
namespace fs = fs0::language::fstrips;

namespace fs0 {

    class AtomIndex;

    //! An iterator that models action schema applicability as an action CSP.
    //! The iterator receives an (ordered) set of lifted-action CSP handlers, and upon iteration
    //! returns, chainedly, each of the lifted-action IDs that are applicable.
    class SDDActionIterator {
    protected:
        std::reference_wrapper<const State> state_;

        std::reference_wrapper<const std::vector<std::shared_ptr<ActionSchemaSDD>>> sdds_;

    public:
        SDDActionIterator() {}
        SDDActionIterator(const State& state, const std::vector<std::shared_ptr<ActionSchemaSDD>>& sdds, const AtomIndex& tuple_index);

        constexpr SDDActionIterator& operator=(const SDDActionIterator&) = default;
        SDDActionIterator& operator=(SDDActionIterator&&) noexcept = default;

        class Iterator {
            friend class SDDActionIterator;

        public:

            ~Iterator();

        protected:
            Iterator(const State& state, const std::vector<std::shared_ptr<ActionSchemaSDD>>& sdds, unsigned currentIdx);

            std::reference_wrapper<const State> state_;

            std::reference_wrapper<const std::vector<std::shared_ptr<ActionSchemaSDD>>> sdds_;

            unsigned current_sdd_idx_;

            SddNode* current_sdd_;

            bool current_models_computed_;

            LiftedActionID* _action;

            std::vector<SDDModel> current_resultset_;
            unsigned current_resultset_idx_;

            //! Advance into the next SDD model
            void advance();

        public:
            const Iterator& operator++() {
                advance();
                return *this;
            }
            const Iterator operator++(int) {Iterator tmp(*this); operator++(); return tmp;}

            const LiftedActionID& operator*() const { return *_action; }

            //! This is not really true... but will work for the purpose of comparing with the end iterator.
            bool operator==(const Iterator &other) const { return current_sdd_idx_ == other.current_sdd_idx_; }
            bool operator!=(const Iterator &other) const { return !(this->operator==(other)); }
        };

        Iterator begin() const { return {state_, sdds_, 0}; }
        Iterator end() const { return {state_, sdds_, (unsigned int) sdds_.get().size()}; }
    };


} // namespaces

