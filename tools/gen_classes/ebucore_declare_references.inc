{ &MXF_ITEM_K(ebucoreMainFramework, metadataSchemeInformation), "ebucoreMetadataSchemeInformation*" },

{ &MXF_ITEM_K(ebucoreMetadataSchemeInformation, ebucoreMetadataProvider), "ebucoreEntity*" },

{ &MXF_ITEM_K(ebucoreEntity, entityContact), "ebucoreContact*" },
{ &MXF_ITEM_K(ebucoreEntity, entityOrganisation), "ebucoreOrganisation*" },
{ &MXF_ITEM_K(ebucoreEntity, entityRole), "ebucoreRole*" },

{ &MXF_ITEM_K(ebucoreContact, contactType), "ebucoreTypeGroup*" },
{ &MXF_ITEM_K(ebucoreContact, contactDetails), "ebucoreContactDetails*" },
{ &MXF_ITEM_K(ebucoreContact, contactRelatedContacts), "ebucoreContact*" },

{ &MXF_ITEM_K(ebucoreContactDetails, detailsType), "ebucoreTypeGroup*" },
{ &MXF_ITEM_K(ebucoreContactDetails, address), "ebucoreAddress*" },



{ &MXF_ITEM_K(ebucoreOrganisation, organisationType), "ebucoreTypeGroup*" },
{ &MXF_ITEM_K(ebucoreOrganisation, organisationDetails), "ebucoreContactDetails*" },
{ &MXF_ITEM_K(ebucoreOrganisation, organisationRelatedContacts), "ebucoreContact*" },

{ &MXF_ITEM_K(ebucoreRole, roleType), "ebucoreTypeGroup*" },

{ &MXF_ITEM_K(ebucoreCoreMetadata, identifier), "ebucoreIdentifier*" },
{ &MXF_ITEM_K(ebucoreCoreMetadata, title), "ebucoreTitle*" },
{ &MXF_ITEM_K(ebucoreCoreMetadata, alternativeTitle), "ebucoreAlternativeTitle*" },
{ &MXF_ITEM_K(ebucoreCoreMetadata, creator), "ebucoreEntity*" },
{ &MXF_ITEM_K(ebucoreCoreMetadata, subject), "ebucoreSubject*" },
{ &MXF_ITEM_K(ebucoreCoreMetadata, description), "ebucoreDescription*" },
{ &MXF_ITEM_K(ebucoreCoreMetadata, publisher), "ebucoreEntity*" },
{ &MXF_ITEM_K(ebucoreCoreMetadata, contributor), "ebucoreEntity*" },
{ &MXF_ITEM_K(ebucoreCoreMetadata, date), "ebucoreDate*" },
{ &MXF_ITEM_K(ebucoreCoreMetadata, type), "ebucoreType*" },
{ &MXF_ITEM_K(ebucoreCoreMetadata, language), "ebucoreLanguage*" },
{ &MXF_ITEM_K(ebucoreCoreMetadata, coverage), "ebucoreCoverage*" },
{ &MXF_ITEM_K(ebucoreCoreMetadata, rights), "ebucoreRights*" },
{ &MXF_ITEM_K(ebucoreCoreMetadata, rating), "ebucoreRating*" },
{ &MXF_ITEM_K(ebucoreCoreMetadata, version), "ebucoreVersion*" },
{ &MXF_ITEM_K(ebucoreCoreMetadata, publicationHistoryEvent), "ebucorePublicationHistoryEvent*" },
{ &MXF_ITEM_K(ebucoreCoreMetadata, customRelation), "ebucoreCustomRelation*" },
{ &MXF_ITEM_K(ebucoreCoreMetadata, basicRelation), "ebucoreBasicRelation*" },

{ &MXF_ITEM_K(ebucoreIdentifier, identifierTypeGroup), "ebucoreTypeGroup*" },
{ &MXF_ITEM_K(ebucoreIdentifier, identifierFormatGroup), "ebucoreFormatGroup*" },
{ &MXF_ITEM_K(ebucoreIdentifier, identifierAttributorEntity), "ebucoreEntity*" },


{ &MXF_ITEM_K(ebucoreAlternativeTitle, alternativeTitleTypeGroup), "ebucoreTypeGroup*" },
{ &MXF_ITEM_K(ebucoreAlternativeTitle, alternativeTitleStatusGroup), "ebucoreStatusGroup*" },



{ &MXF_ITEM_K(ebucoreSubject, subjectTypeGroup), "ebucoreTypeGroup*" },
{ &MXF_ITEM_K(ebucoreSubject, ubjectAttributorEntity), "ebucoreEntity*" },

{ &MXF_ITEM_K(ebucoreDescription, descriptionTypeGroup), "ebucoreTypeGroup*" },

{ &MXF_ITEM_K(ebucoreDate, dateTypeGroup), "ebucoreTypeGroup*" },

{ &MXF_ITEM_K(ebucoreType, objectType), "ebucoreObjectType*" },
{ &MXF_ITEM_K(ebucoreType, genre), "ebucoreGenre*" },
{ &MXF_ITEM_K(ebucoreType, targetAudience), "ebucoreTargetAudience*" },

{ &MXF_ITEM_K(ebucoreObjectType, objectTypeGroup), "ebucoreTypeGroup*" },

{ &MXF_ITEM_K(ebucoreGenre, genreKindGroup), "ebucoreTypeGroup*" },

{ &MXF_ITEM_K(ebucoreTargetAudience, targetAudienceKindGroup), "ebucoreTypeGroup*" },

{ &MXF_ITEM_K(ebucoreLanguage, languagePurposeSet), "ebucoreLangaugePurpose*" },

{ &MXF_ITEM_K(ebucoreLanguagePurpose, languagePurposeTypeGroup), "ebucoreTypeGroup*" },

{ &MXF_ITEM_K(ebucoreCoverage, spatial), "ebucoreSpatial*" },
{ &MXF_ITEM_K(ebucoreCoverage, temporal), "ebucoreTemporal*" },

{ &MXF_ITEM_K(ebucoreSpatial, location), "ebucoreLocation*" },
{ &MXF_ITEM_K(ebucoreSpatial, coordinates), "ebucoreCoordinates*" },

{ &MXF_ITEM_K(ebucoreLocation, locationKindGroup), "ebucoreTypeGroup*" },

{ &MXF_ITEM_K(ebucoreCoordinates, coordinatesKindGroup), "ebucoreTypeGroup*" },

{ &MXF_ITEM_K(ebucoreTemporal, periodOfTime), "ebucorePeriodOfTime*" },

{ &MXF_ITEM_K(ebucorePeriodOfTime, periodKindGroup), "ebucoreTypeGroup*" },

{ &MXF_ITEM_K(ebucoreRights, rightsCoverage), "ebucoreCoverage*" },
{ &MXF_ITEM_K(ebucoreRights, rightsHolderEntity), "ebucoreEntity*" },
{ &MXF_ITEM_K(ebucoreRights, rightsContacts), "ebucoreContact*" },
{ &MXF_ITEM_K(ebucoreRights, rightsKindGroup), "ebucoreTypeGroup*" },


{ &MXF_ITEM_K(ebucoreRating, ratingTypeGroup), "ebucoreTypeGroup*" },
{ &MXF_ITEM_K(ebucoreRating, ratingFormatGroup), "ebucoreFormatGroup*" },
{ &MXF_ITEM_K(ebucoreRating, ratingProviderEntity), "ebucoreEntity*" },


{ &MXF_ITEM_K(ebucoreCustomRelation, customRelationTypeGroup), "ebucoreTypeGroup*" },


