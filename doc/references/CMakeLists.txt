#
# (c) Copyright 2009, Hewlett-Packard Development Company, LP
#
#  See the file named COPYING for license details
#
# cmake description for the doc/references directory

# Unfortunately, FILE(GLOB *bib) doesn't do what we want, if a new
# .bib file is added, the file isn't automatically regenerated.
SET(BIB_FILES 
    aadebug.bib
    acmtocs.bib
    acmtoplas.bib
    asplos.bib
    books.bib
    cacm.bib
    csur.bib
    datamation.bib
    disc.bib
    distributedcomputing.bib
    dsn.bib
    eurosys.bib
    fast.bib
    edcc.bib
    efts.bib
    hotdep.bib
    hotos.bib
    icdcs.bib
    icpp.bib
    ic.bib
    ics.bib
    ieee-computer.bib
    ieeejsac.bib
    ieee-icin.bib
    ieeetoc.bib
    ieeetpds.bib
    isca.bib
    jacm.bib
    journal-high-perf-compute-apps.bib
    mascots.bib
    middleware.bib
    nsdi.bib
    osdi.bib
    os-review.bib
    pldi.bib
    podc.bib
    rfcs.bib
    scientific-programming.bib
    siam-parallel-scientific.bib
    sigact.bib
    sigcomm.bib
    sigmetrics.bib
    sigmod.bib
    socc.bib
    sosp.bib
    sss.bib
    supercomputing-review.bib
    tcs.bib
    tcsc.bib
    tocs.bib
    tods.bib
    toit.bib
    toplas.bib
    tr.bib
    tse.bib
    usenix.bib
    vldb.bib
    web-sites.bib
    wmcsa.bib
    wmrd.bib
)

ADD_CUSTOM_TARGET(all-bib ALL 
                  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/build-bibs.sh
			  lintel-latex-all.bib
		          ${CMAKE_CURRENT_SOURCE_DIR} 
			  ${BIB_FILES}  venues-long.bib
		  DEPENDS ${BIB_FILES} venues-long.bib)

ADD_CUSTOM_TARGET(config-bib ALL 
                  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/build-bibs.sh
			  lintel-latex-bib.bib
		          ${CMAKE_CURRENT_SOURCE_DIR} 
			  ${BIB_FILES} 
		  DEPENDS ${BIB_FILES} )

INSTALL(FILES ${BIB_FILES} ${CMAKE_CURRENT_BINARY_DIR}/lintel-latex-all.bib
	      ${CMAKE_CURRENT_BINARY_DIR}/lintel-latex-bib.bib
	      venues-short.bib venues-long.bib
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/lintel-latex-rebuild)

ADD_TEST(web-sites-check ${CMAKE_CURRENT_SOURCE_DIR}/check-web-sites-bib.sh)
