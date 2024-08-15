#ifndef MULTI_RAT_FILEPROTOCOL_HELPER_H
#define MULTI_RAT_FILEPROTOCOL_HELPER_H

#include <map>
#include "ns3/object.h"
#include "ns3/node-container.h"
#include "multi-rat-fileprotocol.h"

namespace ns3
{

class MultiRatFileProtocol;

/**
 * \ingroup MultiRat
 * @brief Helper for multi-rat file protocol to synchronize routes and node compatibility
 */
class MultiRatFileProtocolHelper : public Object {
  public:

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    MultiRatFileProtocolHelper();

    ~MultiRatFileProtocolHelper() override;

    /**
     * @brief updates the registered protocols periodically.
     */
    void Update();

    /**
     * @brief Updates the routes in the registered protocols based on the set routes file
     */
    void UpdateRoutes();

    /**
     * @brief Set the required files required by the file protocol helper
     * @param execute command used to run the program that determines the routes
     * @param status output file for the protocol status
     * @param routes input file for the routes
     */
    void SetFiles(std::string execute, std::string status, std::string routes);

    /**
     * @brief Write the status of the protocols in the set status file
     */
    void PostStatus();

    /**
     * @brief Iterate over all protocols and set for each one which nodes are compatibile with every channel
     */
    void UpdateCompatibleClients();

    /**
     * @brief Register a protocol to the hlelper
     * @param protocol pointer to protocol
     */
    void RegisterProtocol(Ptr<MultiRatFileProtocol> protocol);

  private:
  

    /**
     * @brief helper function to allow us to set the first update attribute
     */
    void Start();

    std::vector<Ptr<MultiRatFileProtocol>> m_protocols; //fileprotols registered under the helper
    Time m_updaterate; // Time between route updates
    Time m_firstupdate; // Wait time before starting route updates
    EventId m_updateEvent; // event for updating routes
    std::string m_execute; //link to program that will determine routes
    std::string m_status; //file to dump status of fileprotocols
    std::string m_routes; //file to import routes from
};

}

#endif