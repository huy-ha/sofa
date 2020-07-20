/******************************************************************************
*                 SOFA, Simulation Open-Framework Architecture                *
*                    (c) 2006 INRIA, USTL, UJF, CNRS, MGH                     *
*                                                                             *
* This program is free software; you can redistribute it and/or modify it     *
* under the terms of the GNU Lesser General Public License as published by    *
* the Free Software Foundation; either version 2.1 of the License, or (at     *
* your option) any later version.                                             *
*                                                                             *
* This program is distributed in the hope that it will be useful, but WITHOUT *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       *
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License *
* for more details.                                                           *
*                                                                             *
* You should have received a copy of the GNU Lesser General Public License    *
* along with this program. If not, see <http://www.gnu.org/licenses/>.        *
*******************************************************************************
* Authors: The SOFA Team and external contributors (see Authors.txt)          *
*                                                                             *
* Contact information: contact@sofa-framework.org                             *
******************************************************************************/
#define SOFA_CORE_OBJECTMODEL_BASE_CPP
#include <sofa/core/objectmodel/Base.h>
#include <sofa/helper/Factory.h>
#include <sofa/core/ObjectFactory.h>
#include <sofa/helper/logging/Messaging.h>
using sofa::helper::logging::MessageDispatcher ;
using sofa::helper::logging::Message ;

#include <map>
#include <typeinfo>
#include <cstring>
#include <sstream>

#define ERROR_LOG_SIZE 100

namespace sofa
{

namespace core
{

namespace objectmodel
{

using std::string;
static const std::string unnamed_label=std::string("unnamed");

Base::Base()
    : ref_counter(0)
    , serr(_serr)
    , sout(_sout)
    , name(initData(&name,unnamed_label,"name","object name"))
    , f_printLog(initData(&f_printLog, false, "printLog", "if true, emits extra messages at runtime."))
    , f_tags(initData( &f_tags, "tags", "list of the subsets the objet belongs to"))
    , f_bbox(initData( &f_bbox, "bbox", "this object bounding box"))
    , d_componentstate(initData(&d_componentstate, ComponentState::Undefined, "componentState", "The state of the component among (Dirty, Valid, Undefined, Loading, Invalid)."))
{
    name.setOwnerClass("Base");
    name.setAutoLink(false);
    d_componentstate.setAutoLink(false);
    d_componentstate.setReadOnly(true);
    d_componentstate.setOwnerClass("Base");
    f_printLog.setOwnerClass("Base");
    f_printLog.setAutoLink(false);
    f_tags.setOwnerClass("Base");
    f_tags.setAutoLink(false);
    f_bbox.setOwnerClass("Base");
    f_bbox.setReadOnly(true);
    f_bbox.setDisplayed(false);
    f_bbox.setAutoLink(false);
    sendl.setParent(this);

    /// name change => component state update
    addUpdateCallback("name", {&name}, [this](const DataTracker&){
        /// Increment the state counter but without changing the state.
        return m_componentstate.getValue();
    }, {&m_componentstate});
}

Base::~Base()
{
}

void Base::addRef()
{
    ++ref_counter;
}

void Base::release()
{
    if (ref_counter.fetch_sub(1) == 1)
    {
        delete this;
    }
}


void Base::addUpdateCallback(const std::string& name,
                             std::initializer_list<BaseData*> inputs,
                             std::function<sofa::core::objectmodel::ComponentState(const DataTracker&)> func,
                             std::initializer_list<BaseData*> outputs)
{
    // But what if 2 callback functions return 2 different states?
    // won't the 2nd overwrite the state set by the second, potentially masking the invalidity of the component?
    auto& engine = m_internalEngine[name];
    engine.setOwner(this);
    engine.addInputs(inputs);
    engine.setCallback([func, name](const DataTracker& tracker) {
        return func(tracker);
    });
    engine.addOutputs(outputs);

    for(auto& i : engine.getInputs())
        if( i == &d_componentstate ) {
            msg_error(this) << "The componentstate cannot be set as an input of a callbackEngine.";
            engine.delInput(&d_componentstate);
        }
    engine.addOutput(&d_componentstate);
}

void Base::addOutputToCallback(const std::string& name, BaseData* output)
{
    if (m_internalEngine.find(name) != m_internalEngine.end())
        m_internalEngine[name].addOutputs({output});
}


/// Helper method used by initData()
void Base::initData0( BaseData* field, BaseData::BaseInitData& res, const char* name, const char* help, bool isDisplayed, bool isReadOnly )
{
    BaseData::DataFlags flags = BaseData::FLAG_DEFAULT;
    if(isDisplayed) flags |= BaseData::DataFlags(BaseData::FLAG_DISPLAYED); else flags &= ~BaseData::DataFlags(BaseData::FLAG_DISPLAYED);
    if(isReadOnly)  flags |= BaseData::DataFlags(BaseData::FLAG_READONLY); else flags &= ~BaseData::DataFlags(BaseData::FLAG_READONLY);

    initData0(field, res, name, help, flags);
}

/// Helper method used by initData()
void Base::initData0( BaseData* field, BaseData::BaseInitData& res, const char* name, const char* help, BaseData::DataFlags dataFlags )
{
    // Questionnable optimization: test a single 'uint32_t' rather that four 'char'
    static const char *draw_str = "draw";
    static const char *show_str = "show";
    static uint32_t draw_prefix = *reinterpret_cast<const uint32_t*>(draw_str);
    static uint32_t show_prefix = *reinterpret_cast<const uint32_t*>(show_str);

    res.owner = this;
    res.data = field;
    res.name = name;
    res.helpMsg = help;
    res.dataFlags = dataFlags;

    uint32_t prefix = *reinterpret_cast<const uint32_t*>(name);

    if (prefix == draw_prefix || prefix == show_prefix)
        res.group = "Visualization";
}

/// Add a data field.
/// Note that this method should only be called if the field was not initialized with the initData method
void Base::addData(BaseData* f)
{
    addData(f, f->getName());
}

/// Add a data field.
/// Note that this method should only be called if the field was not initialized with the initData method
void Base::addData(BaseData* f, const std::string& name)
{
    if (name.size() > 0 && (findData(name) || findLink(name)))
    {
        msg_warning() << "Data field name " << name
                << " already used in this class or in a parent class !";
    }
    m_vecData.push_back(f);
    m_aliasData.insert(std::make_pair(name, f));
    f->setOwner(this);
}

/// Add an alias to a Data
void Base::addAlias( BaseData* field, const char* alias)
{
    m_aliasData.insert(std::make_pair(std::string(alias),field));
}

/// Add a link.
/// Note that this method should only be called if the link was not initialized with the initLink method
void Base::addLink(BaseLink* l)
{
    const std::string& name = l->getName();
    if (name.size() > 0 && (findData(name) || findLink(name)))
    {
        msg_warning() << "Link name '" << name
                << "' already used in this class or in a parent class !";
    }
    m_vecLink.push_back(l);
    m_aliasLink.insert(std::make_pair(name, l));
}

/// Add an alias to a Link
void Base::addAlias( BaseLink* link, const char* alias)
{
    m_aliasLink.insert(std::make_pair(std::string(alias),link));
}

/// Get the type name of this object (i.e. class and template types)
std::string Base::getTypeName() const
{
    return getClass()->typeName;
}

/// Get the class name of this object
std::string Base::getClassName() const
{
    return getClass()->className;
}

/// Get the template type names (if any) used to instantiate this object
std::string Base::getTemplateName() const
{
    return getClass()->templateName;
}

/// Get the template type names (if any) used to instantiate this object
std::string Base::getNameSpaceName() const
{
    return getClass()->namespaceName;
}

void Base::setName(const std::string& na)
{
    name.setValue(na);
}

/// Set the name of this object, adding an integer counter
void Base::setName(const std::string& n, int counter)
{
    std::ostringstream o;
    o << n << counter;
    setName(o.str());
}

void Base::processStream(std::ostream& out)
{
    if (serr==out)
    {
        MessageDispatcher::log(serr.messageClass(),
                               serr.messageType(), sofa::helper::logging::getComponentInfo(this),
                               serr.fileInfo()) << serr.str() ;
        serr.clear();
    }
    else if (sout==out)
    {
        if (f_printLog.getValue())
        {
            MessageDispatcher::log(sout.messageClass(),
                                   sout.messageType(), sofa::helper::logging::getComponentInfo(this),
                                   sout.fileInfo()) << sout.str();
        }

        sout.clear();
    }
}

void Base::addMessage(const Message &m) const
{
    if(m_messageslog.size() >= ERROR_LOG_SIZE ){
        m_messageslog.pop_front();
    }
    m_messageslog.push_back(m) ;
}

void Base::clearLoggedMessages() const
{
   m_messageslog.clear() ;
}


const std::deque<sofa::helper::logging::Message>& Base::getLoggedMessages() const
{
    return m_messageslog ;
}

const std::string Base::getLoggedMessagesAsString(const sofa::helper::logging::Message::TypeSet t) const
{
    std::stringstream tmpstr ;
    for(Message& m : m_messageslog){
        if( t.find(m.type()) !=  t.end() )
        {
            tmpstr << m.type() << ":" <<  m.messageAsString() << std::endl;
        }
    }
    return tmpstr.str();
}

size_t Base::countLoggedMessages(const sofa::helper::logging::Message::TypeSet t) const
{
    size_t tmp=0;
    for(Message& m : m_messageslog){
        if( t.find(m.type()) !=  t.end() )
        {
            tmp++;
        }
    }
    return tmp;
}


bool Base::hasTag(Tag t) const
{
    return (f_tags.getValue().count( t ) > 0 );
}


void Base::addTag(Tag t)
{
    f_tags.beginEdit()->insert(t);
    f_tags.endEdit();
}

void Base::removeTag(Tag t)
{
    f_tags.beginEdit()->erase(t);
    f_tags.endEdit();
}

void Base::removeData(BaseData* d)
{
    m_vecData.erase(std::find(m_vecData.begin(), m_vecData.end(), d));
    auto range = m_aliasData.equal_range(d->getName());
    m_aliasData.erase(range.first, range.second);
}

/// Find a data field given its name.
/// Return nullptr if not found. If more than one field is found (due to aliases), only the first is returned.
BaseData* Base::findData( const std::string &name ) const
{
    //Search in the aliases
    if(m_aliasData.size())
    {
        auto range = m_aliasData.equal_range(name);
        if (range.first != range.second)
            return range.first->second;
        else
            return nullptr;
    }
    else return nullptr;
}

/// Find fields given a name: several can be found as we look into the alias map
std::vector< BaseData* > Base::findGlobalField( const std::string &name ) const
{
    std::vector<BaseData*> result;
    //Search in the aliases
    auto range = m_aliasData.equal_range(name);
    for (auto itAlias=range.first; itAlias!=range.second; ++itAlias)
        result.push_back(itAlias->second);
    return result;
}


/// Find a link given its name.
/// Return nullptr if not found. If more than one link is found (due to aliases), only the first is returned.
BaseLink* Base::findLink( const std::string &name ) const
{
    //Search in the aliases
    auto range = m_aliasLink.equal_range(name);
    if (range.first != range.second)
        return range.first->second;
    else
        return nullptr;
}

/// Find links given a name: several can be found as we look into the alias map
std::vector< BaseLink* > Base::findLinks( const std::string &name ) const
{
    std::vector<BaseLink*> result;
    //Search in the aliases
    auto range = m_aliasLink.equal_range(name);
    for (auto itAlias=range.first; itAlias!=range.second; ++itAlias)
        result.push_back(itAlias->second);
    return result;
}

bool Base::findDataLinkDest(BaseData*& ptr, const std::string& path, const BaseLink* link)
{
    std::string pathStr, dataStr;
    if (link)
    {
        if (!link->parseString(path, &pathStr, &dataStr))
            return false;
    }
    else
    {
        if (!BaseLink::ParseString(path, &pathStr, &dataStr, this))
            return false;
    }
    if (pathStr.empty() || pathStr == std::string("[]"))
    {
        ptr = this->findData(dataStr);
        return (ptr != nullptr);
    }
    Base* obj = nullptr;
    if (!findLinkDest(obj, BaseLink::CreateString(pathStr), link))
        return false;
    if (!obj)
        return false;
    ptr = obj->findData(dataStr);
    return (ptr != nullptr);
}

void* Base::findLinkDestClass(const BaseClass* /*destType*/, const std::string& /*path*/, const BaseLink* /*link*/)
{
    msg_error() << "Base: calling unimplemented findLinkDest method" ;
    return nullptr;
}

bool Base::hasField( const std::string& attribute) const
{
    return m_aliasData.find(attribute) != m_aliasData.end()
            || m_aliasLink.find(attribute) != m_aliasLink.end();
}

/// Assign one field value (Data or Link)
bool Base::parseField( const std::string& attribute, const std::string& value)
{
    std::vector< BaseData* > dataVec = findGlobalField(attribute);
    std::vector< BaseLink* > linkVec = findLinks(attribute);
    if (dataVec.empty() && linkVec.empty())
    {
        msg_warning() << "Unknown Data field or Link: " << attribute ;
        return false; // no field found
    }
    bool ok = true;

    for (unsigned int d=0; d<dataVec.size(); ++d)
    {
        // test if data is a link and can be linked
        if (value.length() > 0 && value[0] == '@' && dataVec[d]->canBeLinked())
        {
            if (!dataVec[d]->setParent(value))
            {
                BaseData* data = nullptr;
                BaseLink* bl = nullptr;
                dataVec[d]->findDataLinkDest(data, value, bl);
                if (data != nullptr && dynamic_cast<EmptyData*>(data) != nullptr)
                {
                    Base* owner = data->getOwner();
                    DDGNode* o = dynamic_cast<DDGNode*>(owner);
                    o->delOutput(data);
                    owner->removeData(data);
                    BaseData* newBD = dataVec[d]->getNewInstance();
                    newBD->setName(data->getName());
                    owner->addData(newBD);
                    newBD->setGroup("Outputs");
                    o->addOutput(newBD);
                    dataVec[d]->setParent(newBD);
                    ok = true;
                    continue;
                }
                msg_warning()<<"Could not setup Data link between "<< value << " and " << attribute << "." ;
                ok = false;
                continue;
            }
            else
            {
                BaseData* parentData = dataVec[d]->getParent();
                msg_info() << "Link from parent Data " << value << " (" << parentData->getValueTypeInfo()->name() << ") to Data " << attribute << "(" << dataVec[d]->getValueTypeInfo()->name() << ") OK";
            }
            /* children Data cannot be modified changing the parent Data value */
            dataVec[d]->setReadOnly(true);
            continue;
        }
        if( !(dataVec[d]->read( value )) && !value.empty())
        {
            msg_warning()<<"Could not read value for data field "<< attribute <<": " << value ;
            ok = false;
        }
    }
    for (unsigned int l=0; l<linkVec.size(); ++l)
    {
        if( !(linkVec[l]->read( value )) && !value.empty())
        {
            msg_warning()<<"Could not read value for link "<< attribute <<": " << value;
            ok = false;
        }
        msg_info() << "Link " << linkVec[l]->getName() << " = " << linkVec[l]->getValueString();
        unsigned int s = unsigned(linkVec[l]->getSize());
        for (unsigned int i=0; i<s; ++i)
        {
            std::stringstream tmp;
            tmp << "  " << linkVec[l]->getLinkedPath(i) << " = ";
            Base* b = linkVec[l]->getLinkedBase(i);
            BaseData* d = linkVec[l]->getLinkedData(i);
            if (b) tmp << b->getTypeName() << " " << b->getName();
            if (d) tmp << " . " << d->getValueTypeString() << " " << d->getName();
            msg_info() << tmp.str();
        }
    }
    return ok;
}

void  Base::parseFields ( const std::list<std::string>& str )
{
    string name,value;
    std::list<std::string>::const_iterator it = str.begin(), itend = str.end();
    while(it != itend)
    {
        name = *it;
        ++it;
        if (it == itend) break;
        value = *it;
        ++it;
        parseField(name, value);
    }
}

void  Base::parseFields ( const std::map<std::string,std::string*>& args )
{
    std::string key,val;
    for( std::map<string,string*>::const_iterator i=args.begin(), iend=args.end(); i!=iend; ++i )
    {
        if( (*i).second!=nullptr )
        {
            key=(*i).first;
            val=*(*i).second;
            parseField(key, val);
        }
    }
}

/// Parse the given description to assign values to this object's fields and potentially other parameters
void  Base::parse ( BaseObjectDescription* arg )
{
    for( auto& it : arg->getAttributeMap() )
    {
        const std::string& attrName = it.first;

        // FIX: "type" is already used to define the type of object to instanciate, any Data with
        // the same name cannot be extracted from BaseObjectDescription
        if (attrName == std::string("type"))
            continue;
        if (!hasField(attrName)) continue;

        parseField(attrName, it.second);
    }
    updateLinks(false);
}

/// Update pointers in case the pointed-to objects have appeared
void Base::updateLinks(bool logErrors)
{
    // update links
    for(VecLink::const_iterator iLink = m_vecLink.begin(); iLink != m_vecLink.end(); ++iLink)
    {
        bool ok = (*iLink)->updateLinks();
        if (!ok && (*iLink)->storePath() && logErrors)
        {
            msg_warning() << "Link update failed for " << (*iLink)->getName() << " = " << (*iLink)->getValueString() ;
        }
    }
}

void  Base::writeDatas ( std::map<std::string,std::string*>& args )
{
    for(VecData::const_iterator iData = m_vecData.begin(); iData != m_vecData.end(); ++iData)
    {
        BaseData* field = *iData;
        std::string name = field->getName();
        if( args[name] != nullptr )
            *args[name] = field->getValueString();
        else
            args[name] =  new string(field->getValueString());
    }
    for(VecLink::const_iterator iLink = m_vecLink.begin(); iLink != m_vecLink.end(); ++iLink)
    {
        BaseLink* link = *iLink;
        std::string name = link->getName();
        if( args[name] != nullptr )
            *args[name] = link->getValueString();
        else
            args[name] =  new string(link->getValueString());
    }
}

static std::string xmlencode(const std::string& str)
{
    std::string res;
    for (unsigned int i=0; i<str.length(); ++i)
    {
        switch(str[i])
        {
        case '<': res += "&lt;"; break;
        case '>': res += "&gt;"; break;
        case '&': res += "&amp;"; break;
        case '"': res += "&quot;"; break;
        case '\'': res += "&apos;"; break;
        default:  res += str[i];
        }
    }
    return res;
}

void  Base::writeDatas (std::ostream& out, const std::string& separator)
{
    for(VecData::const_iterator iData = m_vecData.begin(); iData != m_vecData.end(); ++iData)
    {
        BaseData* field = *iData;
        if (!field->getLinkPath().empty() )
        {
            out << separator << field->getName() << "=\""<< xmlencode(field->getLinkPath()) << "\" ";
        }
        else
        {
            if(field->isPersistent() && field->isSet())
            {
                std::string val = field->getValueString();
                if (!val.empty())
                    out << separator << field->getName() << "=\""<< xmlencode(val) << "\" ";
            }
        }
    }
    for(VecLink::const_iterator iLink = m_vecLink.begin(); iLink != m_vecLink.end(); ++iLink)
    {
        BaseLink* link = *iLink;
        if(link->storePath())
        {
            std::string val = link->getValueString();
            if (!val.empty())
                out << separator << link->getName() << "=\""<< xmlencode(val) << "\" ";
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////// DEPRECATED SECTION ///////////////////////////////////////////////////
/// Everything below this point is deprecated and will be removed soon !
/// Do not use it anymore. For each function a replacement is suggested.

const std::string Base::getWarnings() const
{
    dmsg_deprecated(this) << " getWarning() is deprecated."
                             " Using deprecated code may result in lower performances or un-expected behavior."
                             " To remove this warning you need to use getLoggedMessage() instead. ";

    std::stringstream tmpstr ;
    for(Message& m : m_messageslog){
        if(m.type()==Message::Error || m.type()==Message::Warning || m.type()==Message::Fatal)
        {
            tmpstr << m.messageAsString() ;
        }
    }
    return tmpstr.str();
}

const std::string Base::getOutputs() const
{
    dmsg_deprecated(this) <<  " getOutputs() is deprecated."
                              " Using deprecated code may result in lower performances or un-expected behavior."
                              " To remove this warning you need to use getLoggedMessage() instead. ";

    std::stringstream tmpstr ;
    for(Message& m : m_messageslog){
        if(m.type()==Message::Info || m.type()==Message::Advice || m.type()==Message::Deprecated){
            tmpstr << m.messageAsString() ;
        }
    }
    return tmpstr.str();
}

void Base::clearWarnings()
{
    dmsg_deprecated(this) <<  " clearWarnings() is deprecated."
                              " Using deprecated code may result in lower performances or un-expected behavior."
                              " To remove this warning you need to use clearLoggedMessages() instead. ";
    clearLoggedMessages();
}

void Base::clearOutputs()
{
    dmsg_deprecated(this) <<  " clearOutput() is deprecated."
                              " Using deprecated code may result in lower performances or un-expected behavior."
                              " To remove this warning you need to use clearLoggedMessages() instead. ";
    clearLoggedMessages();
}

/// Set the source filename (where the component is implemented)
void Base::setDefinitionSourceFileName(const std::string& sourceFileName)
{
    m_definitionSourceFileName = sourceFileName;
}

/// Get the source filename (where the component is implemented)
const std::string& Base::getDefinitionSourceFileName() const
{
    return m_definitionSourceFileName;
}

/// Set the source location (where the component is implemented)
void Base::setDefinitionSourceFilePos(const int linenum)
{
    m_definitionSourceFilePos = linenum;
}

/// Get the source location (where the component is implemented)
int Base::getDefinitionSourceFilePos() const
{
    return m_definitionSourceFilePos;
}

/// Set the file where the instance has been created
/// This is useful to store where the component was emitted from
void Base::setInstanciationSourceFileName(const std::string& filename)
{
    m_instanciationSourceFileName = filename;
}

/// Get the file where the instance has been created
/// This is useful to store where the component was emitted from
const std::string& Base::getInstanciationSourceFileName() const
{
    return m_instanciationSourceFileName;
}

/// Set the file location (line number) where the instance has been created
/// This is useful to store where the component was emitted from
void Base::setInstanciationSourceFilePos(const int lineco)
{
    m_instanciationSourceFilePos = lineco;
}

/// Get the file location (line number) where the instance has been created
/// This is useful to store where the component was emitted from
int Base::getInstanciationSourceFilePos() const
{
    return m_instanciationSourceFilePos;
}




} // namespace objectmodel

} // namespace core

namespace helper
{
namespace logging
{

SofaComponentInfo::SofaComponentInfo(const sofa::core::objectmodel::Base* c)
{
    assert(c!=nullptr) ;
    m_component = c ;
    m_sender = c->getClassName() ;
    m_name = c->getName() ;
}

} // namespace logging
} // namespace helper
} // namespace sofa
